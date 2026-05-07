// Copyright (c) Myika AI. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "HAL/CriticalSection.h"
#include "Interfaces/IPv4/IPv4Endpoint.h"

class FTcpListener;
class FMyikaMCPConnection;

/**
 * Owns the TCP listener and the set of active connections. Methods are safe to call
 * from the game thread; internal connection threads are managed automatically.
 */
class FMyikaMCPServer
{
public:
	FMyikaMCPServer();
	~FMyikaMCPServer();

	/** Bind to the given host (e.g. "127.0.0.1" or "0.0.0.0") and port. */
	bool Start(const FString& Host, int32 Port);
	void Stop();
	bool IsRunning() const;

	int32 GetPort() const { return BoundPort; }
	const FString& GetHost() const { return BoundHost; }
	int32 GetConnectionCount() const;
	FString GetStatusString() const;

private:
	bool OnIncomingConnection(class FSocket* Socket, const FIPv4Endpoint& Endpoint);
	void ReapStoppedConnections();

	TUniquePtr<FTcpListener> Listener;
	FString BoundHost;
	int32 BoundPort = 0;
	mutable FCriticalSection ConnectionsMutex;
	TArray<TUniquePtr<FMyikaMCPConnection>> Connections;
};
