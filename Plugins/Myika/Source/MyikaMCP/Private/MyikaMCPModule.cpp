// Copyright (c) Myika AI. All rights reserved.

#include "MyikaMCPModule.h"
#include "MyikaMCPLog.h"
#include "MyikaMCPServer.h"
#include "MyikaMCPSettings.h"
#include "MyikaMCPToolbar.h"
#include "Commands/MyikaMCPCommands.h"
#include "Modules/ModuleManager.h"
#include "ToolMenus.h"

DEFINE_LOG_CATEGORY(LogMyikaMCP);

IMPLEMENT_MODULE(FMyikaMCPModule, MyikaMCP)

FMyikaMCPModule& FMyikaMCPModule::Get()
{
	return FModuleManager::LoadModuleChecked<FMyikaMCPModule>("MyikaMCP");
}

void FMyikaMCPModule::StartupModule()
{
	UE_LOG(LogMyikaMCP, Log, TEXT("MyikaMCP module starting."));

	RegisterCommands();

	Server = MakeShared<FMyikaMCPServer>();

	UToolMenus::RegisterStartupCallback(FSimpleMulticastDelegate::FDelegate::CreateLambda([this]()
	{
		Toolbar = MakeShared<FMyikaMCPToolbarExtension>();
		Toolbar->Register();

		const UMyikaMCPSettings* Settings = GetDefault<UMyikaMCPSettings>();
		if (Settings && Settings->bAutoStartOnEditorLaunch)
		{
			StartServer();
		}
	}));
}

void FMyikaMCPModule::ShutdownModule()
{
	UE_LOG(LogMyikaMCP, Log, TEXT("MyikaMCP module shutting down."));

	if (Toolbar.IsValid())
	{
		Toolbar->Unregister();
		Toolbar.Reset();
	}

	StopServer();
	Server.Reset();

	MyikaMCP::Commands::UnregisterAll();
}

void FMyikaMCPModule::RegisterCommands()
{
	MyikaMCP::Commands::RegisterAll();
}

bool FMyikaMCPModule::StartServer()
{
	if (!Server.IsValid())
	{
		Server = MakeShared<FMyikaMCPServer>();
	}
	if (Server->IsRunning())
	{
		return true;
	}
	const UMyikaMCPSettings* Settings = GetDefault<UMyikaMCPSettings>();
	const FString Host = (Settings && !Settings->Host.IsEmpty()) ? Settings->Host : FString(TEXT("127.0.0.1"));
	const int32 Port = Settings ? Settings->Port : 13379;
	return Server->Start(Host, Port);
}

void FMyikaMCPModule::StopServer()
{
	if (Server.IsValid() && Server->IsRunning())
	{
		Server->Stop();
	}
}

bool FMyikaMCPModule::IsServerRunning() const
{
	return Server.IsValid() && Server->IsRunning();
}

FString FMyikaMCPModule::GetStatusString() const
{
	if (!Server.IsValid())
	{
		return TEXT("uninitialized");
	}
	return Server->GetStatusString();
}
