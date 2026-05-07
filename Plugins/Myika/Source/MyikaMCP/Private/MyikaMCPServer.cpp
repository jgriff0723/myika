// Copyright (c) Myika AI. All rights reserved.

#include "MyikaMCPServer.h"
#include "MyikaMCPConnection.h"
#include "MyikaMCPLog.h"
#include "Common/TcpListener.h"
#include "Interfaces/IPv4/IPv4Address.h"

FMyikaMCPServer::FMyikaMCPServer() = default;

FMyikaMCPServer::~FMyikaMCPServer()
{
	Stop();
}

bool FMyikaMCPServer::Start(const FString& Host, int32 Port)
{
	if (Listener.IsValid())
	{
		UE_LOG(LogMyikaMCP, Warning, TEXT("Start called while server already running on %s:%d"), *BoundHost, BoundPort);
		return true;
	}

	FIPv4Address Address;
	if (!FIPv4Address::Parse(Host, Address))
	{
		UE_LOG(LogMyikaMCP, Error, TEXT("Invalid Host '%s' — expected an IPv4 literal like 127.0.0.1 or 0.0.0.0"), *Host);
		return false;
	}

	const FIPv4Endpoint Endpoint(Address, static_cast<uint16>(Port));
	Listener = MakeUnique<FTcpListener>(Endpoint);

	// FTcpListener binds and spawns its thread in the constructor; verify it succeeded.
	if (!Listener->IsActive())
	{
		UE_LOG(LogMyikaMCP, Error, TEXT("Failed to bind TCP listener on %s:%d (port in use?)"), *Host, Port);
		Listener.Reset();
		return false;
	}

	Listener->OnConnectionAccepted().BindRaw(this, &FMyikaMCPServer::OnIncomingConnection);
	BoundHost = Host;
	BoundPort = Port;
	UE_LOG(LogMyikaMCP, Log, TEXT("Listening on %s:%d"), *Host, Port);
	return true;
}

void FMyikaMCPServer::Stop()
{
	if (Listener.IsValid())
	{
		Listener->Stop();
		Listener.Reset();
		UE_LOG(LogMyikaMCP, Log, TEXT("Listener stopped."));
	}

	{
		FScopeLock Lock(&ConnectionsMutex);
		for (TUniquePtr<FMyikaMCPConnection>& Conn : Connections)
		{
			if (Conn.IsValid())
			{
				Conn->RequestStop();
			}
		}
		Connections.Empty();
	}
	BoundHost.Reset();
	BoundPort = 0;
}

bool FMyikaMCPServer::IsRunning() const
{
	return Listener.IsValid();
}

int32 FMyikaMCPServer::GetConnectionCount() const
{
	FScopeLock Lock(&ConnectionsMutex);
	int32 Live = 0;
	for (const TUniquePtr<FMyikaMCPConnection>& Conn : Connections)
	{
		if (Conn.IsValid() && Conn->IsRunning())
		{
			++Live;
		}
	}
	return Live;
}

FString FMyikaMCPServer::GetStatusString() const
{
	if (!IsRunning())
	{
		return TEXT("stopped");
	}
	return FString::Printf(TEXT("listening %s:%d, %d client(s)"), *BoundHost, BoundPort, GetConnectionCount());
}

bool FMyikaMCPServer::OnIncomingConnection(FSocket* Socket, const FIPv4Endpoint& Endpoint)
{
	if (!Socket)
	{
		return false;
	}
	const FString ClientName = Endpoint.ToString();
	UE_LOG(LogMyikaMCP, Log, TEXT("Accepted connection from %s"), *ClientName);

	{
		FScopeLock Lock(&ConnectionsMutex);
		Connections.Add(MakeUnique<FMyikaMCPConnection>(Socket, ClientName));
	}

	ReapStoppedConnections();
	return true; // Listener keeps ownership-transfer semantics: returning true means we took the socket.
}

void FMyikaMCPServer::ReapStoppedConnections()
{
	FScopeLock Lock(&ConnectionsMutex);
	for (int32 i = Connections.Num() - 1; i >= 0; --i)
	{
		if (!Connections[i].IsValid() || !Connections[i]->IsRunning())
		{
			Connections.RemoveAt(i);
		}
	}
}
