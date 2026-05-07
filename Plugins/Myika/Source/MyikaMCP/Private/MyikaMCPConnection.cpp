// Copyright (c) Myika AI. All rights reserved.

#include "MyikaMCPConnection.h"
#include "MyikaMCPCommandRegistry.h"
#include "MyikaMCPJsonHelpers.h"
#include "MyikaMCPLog.h"
#include "Common/TcpSocketBuilder.h"
#include "HAL/RunnableThread.h"
#include "Sockets.h"
#include "SocketSubsystem.h"

namespace
{
	constexpr int32 MaxFrameBytes = 1 * 1024 * 1024; // 1 MB hard cap on a single message
}

FMyikaMCPConnection::FMyikaMCPConnection(FSocket* InSocket, const FString& InClientName)
	: Socket(InSocket)
	, ClientName(InClientName)
{
	Thread = FRunnableThread::Create(this, *FString::Printf(TEXT("MyikaMCPConn_%s"), *ClientName));
}

FMyikaMCPConnection::~FMyikaMCPConnection()
{
	RequestStop();

	if (Thread)
	{
		Thread->WaitForCompletion();
		delete Thread;
		Thread = nullptr;
	}

	if (Socket)
	{
		Socket->Close();
		ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->DestroySocket(Socket);
		Socket = nullptr;
	}
}

bool FMyikaMCPConnection::Init()
{
	bRunning = true;
	return true;
}

uint32 FMyikaMCPConnection::Run()
{
	UE_LOG(LogMyikaMCP, Log, TEXT("Connection [%s] opened."), *ClientName);

	while (!bStopRequested)
	{
		TArray<uint8> Frame;
		if (!ReadFrame(Frame))
		{
			break;
		}
		HandleFrame(Frame);
	}

	UE_LOG(LogMyikaMCP, Log, TEXT("Connection [%s] closed."), *ClientName);
	bRunning = false;
	return 0;
}

void FMyikaMCPConnection::Stop()
{
	bStopRequested = true;
}

void FMyikaMCPConnection::Exit()
{
	bRunning = false;
}

void FMyikaMCPConnection::RequestStop()
{
	bStopRequested = true;
	if (Socket)
	{
		// Closing the socket unblocks pending Recv on most platforms.
		Socket->Close();
	}
}

bool FMyikaMCPConnection::ReadExact(uint8* Buffer, int32 NumBytes)
{
	// FTcpListener-accepted sockets are non-blocking on Windows. We poll via
	// Wait so we can also notice bStopRequested between recvs.
	int32 TotalRead = 0;
	while (TotalRead < NumBytes && !bStopRequested && Socket)
	{
		if (!Socket->Wait(ESocketWaitConditions::WaitForRead, FTimespan::FromMilliseconds(200)))
		{
			continue; // timeout: no data yet — re-check bStopRequested and loop
		}

		int32 Read = 0;
		const bool bOk = Socket->Recv(Buffer + TotalRead, NumBytes - TotalRead, Read, ESocketReceiveFlags::None);
		if (!bOk)
		{
			// Wait said readable but Recv failed — treat as closed.
			return false;
		}
		if (Read == 0)
		{
			// Wait said readable + Recv succeeded with 0 bytes = peer closed gracefully.
			return false;
		}
		TotalRead += Read;
	}
	return TotalRead == NumBytes;
}

bool FMyikaMCPConnection::WriteExact(const uint8* Buffer, int32 NumBytes)
{
	int32 TotalSent = 0;
	while (TotalSent < NumBytes && !bStopRequested && Socket)
	{
		// Non-blocking sockets: Send returns false / 0 when the kernel buffer
		// is full. Wait for writable, then retry.
		int32 Sent = 0;
		const bool bOk = Socket->Send(Buffer + TotalSent, NumBytes - TotalSent, Sent);
		if (!bOk)
		{
			if (Socket->Wait(ESocketWaitConditions::WaitForWrite, FTimespan::FromMilliseconds(200)))
			{
				continue; // socket became writable — try again
			}
			return false;
		}
		if (Sent == 0)
		{
			if (!Socket->Wait(ESocketWaitConditions::WaitForWrite, FTimespan::FromMilliseconds(200)))
			{
				continue;
			}
			continue;
		}
		TotalSent += Sent;
	}
	return TotalSent == NumBytes;
}

bool FMyikaMCPConnection::ReadFrame(TArray<uint8>& OutBytes)
{
	uint8 Header[4];
	if (!ReadExact(Header, 4))
	{
		return false;
	}
	const uint32 Length =
		(static_cast<uint32>(Header[0]) << 24) |
		(static_cast<uint32>(Header[1]) << 16) |
		(static_cast<uint32>(Header[2]) << 8) |
		(static_cast<uint32>(Header[3]));

	if (Length == 0 || Length > static_cast<uint32>(MaxFrameBytes))
	{
		UE_LOG(LogMyikaMCP, Warning, TEXT("[%s] bad frame length: %u"), *ClientName, Length);
		return false;
	}

	OutBytes.SetNumUninitialized(static_cast<int32>(Length));
	return ReadExact(OutBytes.GetData(), static_cast<int32>(Length));
}

bool FMyikaMCPConnection::WriteFrame(const TArray<uint8>& Bytes)
{
	const uint32 Length = static_cast<uint32>(Bytes.Num());
	uint8 Header[4];
	Header[0] = static_cast<uint8>((Length >> 24) & 0xFF);
	Header[1] = static_cast<uint8>((Length >> 16) & 0xFF);
	Header[2] = static_cast<uint8>((Length >> 8) & 0xFF);
	Header[3] = static_cast<uint8>(Length & 0xFF);
	if (!WriteExact(Header, 4))
	{
		return false;
	}
	return WriteExact(Bytes.GetData(), Bytes.Num());
}

void FMyikaMCPConnection::HandleFrame(const TArray<uint8>& Bytes)
{
	TSharedPtr<FJsonObject> Request;
	if (!MyikaMCP::DeserializeJson(Bytes.GetData(), Bytes.Num(), Request))
	{
		const TSharedRef<FJsonObject> Err = MyikaMCP::BuildError(TEXT(""), TEXT("BAD_FRAME"), TEXT("payload is not valid UTF-8 JSON"));
		TArray<uint8> Out;
		if (MyikaMCP::SerializeJson(Err, Out))
		{
			WriteFrame(Out);
		}
		return;
	}

	FString Id;
	Request->TryGetStringField(TEXT("id"), Id);

	FString Tool;
	if (!Request->TryGetStringField(TEXT("tool"), Tool) || Tool.IsEmpty())
	{
		const TSharedRef<FJsonObject> Err = MyikaMCP::BuildError(Id, TEXT("BAD_FRAME"), TEXT("missing 'tool' field"));
		TArray<uint8> Out;
		if (MyikaMCP::SerializeJson(Err, Out))
		{
			WriteFrame(Out);
		}
		return;
	}

	const TSharedPtr<FJsonObject>* ArgsObjPtr = nullptr;
	TSharedPtr<FJsonObject> ArgsObj;
	if (Request->TryGetObjectField(TEXT("args"), ArgsObjPtr) && ArgsObjPtr && ArgsObjPtr->IsValid())
	{
		ArgsObj = *ArgsObjPtr;
	}
	else
	{
		ArgsObj = MakeShared<FJsonObject>();
	}

	const TSharedRef<FJsonObject> Response = FMyikaMCPCommandRegistry::Get().Dispatch(Id, Tool, ArgsObj);

	TArray<uint8> Out;
	if (MyikaMCP::SerializeJson(Response, Out))
	{
		WriteFrame(Out);
	}
}
