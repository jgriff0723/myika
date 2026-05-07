// Copyright (c) Myika AI. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "HAL/Runnable.h"
#include "HAL/ThreadSafeBool.h"
#include "Sockets.h"

class FRunnableThread;

/**
 * One connection thread per accepted client. Reads length-prefixed framed JSON,
 * dispatches via FMyikaMCPCommandRegistry, writes framed responses.
 */
class FMyikaMCPConnection : public FRunnable
{
public:
	FMyikaMCPConnection(FSocket* InSocket, const FString& InClientName);
	virtual ~FMyikaMCPConnection();

	// FRunnable
	virtual bool Init() override;
	virtual uint32 Run() override;
	virtual void Stop() override;
	virtual void Exit() override;

	void RequestStop();
	bool IsRunning() const { return bRunning; }
	const FString& GetClientName() const { return ClientName; }

private:
	bool ReadExact(uint8* Buffer, int32 NumBytes);
	bool WriteExact(const uint8* Buffer, int32 NumBytes);
	bool ReadFrame(TArray<uint8>& OutBytes);
	bool WriteFrame(const TArray<uint8>& Bytes);
	void HandleFrame(const TArray<uint8>& Bytes);

	FSocket* Socket = nullptr;
	FString ClientName;
	FRunnableThread* Thread = nullptr;
	FThreadSafeBool bStopRequested = false;
	FThreadSafeBool bRunning = false;
};
