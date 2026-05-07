// Copyright (c) Myika AI. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

class IMyikaChatBackend;
class FSpawnTabArgs;
class SDockTab;

class FMyikaMCPChatModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

	static FMyikaMCPChatModule& Get();

	/** Resolve / construct the backend per current settings. Cached for the session; New Chat resets it. */
	TSharedPtr<IMyikaChatBackend> GetOrCreateBackend();

	/** Wipe the current backend and recreate per settings (e.g. after Backend was changed). */
	void RebuildBackend();

	/** Open or focus the dockable chat panel. */
	void OpenChatTab();

private:
	void RegisterTabSpawner();
	void UnregisterTabSpawner();
	TSharedRef<SDockTab> SpawnChatTab(const FSpawnTabArgs& Args);
	void RegisterToolbar();

	TSharedPtr<IMyikaChatBackend> Backend;
};
