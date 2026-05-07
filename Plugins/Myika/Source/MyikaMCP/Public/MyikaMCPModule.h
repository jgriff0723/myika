// Copyright (c) Myika AI. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

class FMyikaMCPServer;
class FMyikaMCPToolbarExtension;

class FMyikaMCPModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

	static FMyikaMCPModule& Get();

	/** Start the TCP server using the configured port. No-op if already running. */
	bool StartServer();

	/** Stop the TCP server. No-op if not running. */
	void StopServer();

	bool IsServerRunning() const;
	FString GetStatusString() const;

private:
	TSharedPtr<FMyikaMCPServer> Server;
	TSharedPtr<FMyikaMCPToolbarExtension> Toolbar;

	void RegisterCommands();
};
