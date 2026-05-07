// Copyright (c) Myika AI. All rights reserved.

#include "MyikaMCPChatModule.h"
#include "MyikaMCPChatLog.h"
#include "MyikaMCPChatSettings.h"
#include "Backends/AnthropicDirectBackend.h"
#include "Backends/ClaudeCodeBackend.h"
#include "MyikaChatBackend.h"
#include "SMyikaChatPanel.h"
#include "Framework/Docking/TabManager.h"
#include "Modules/ModuleManager.h"
#include "ToolMenus.h"
#include "Widgets/Docking/SDockTab.h"
#include "WorkspaceMenuStructure.h"
#include "WorkspaceMenuStructureModule.h"

DEFINE_LOG_CATEGORY(LogMyikaMCPChat);

IMPLEMENT_MODULE(FMyikaMCPChatModule, MyikaMCPChat)

namespace
{
	const FName ChatTabId(TEXT("MyikaMCPChat.Chat"));
}

FMyikaMCPChatModule& FMyikaMCPChatModule::Get()
{
	return FModuleManager::LoadModuleChecked<FMyikaMCPChatModule>("MyikaMCPChat");
}

void FMyikaMCPChatModule::StartupModule()
{
	UE_LOG(LogMyikaMCPChat, Log, TEXT("MyikaMCPChat module starting."));

	UToolMenus::RegisterStartupCallback(FSimpleMulticastDelegate::FDelegate::CreateLambda([this]()
	{
		RegisterTabSpawner();
		RegisterToolbar();
	}));
}

void FMyikaMCPChatModule::ShutdownModule()
{
	UE_LOG(LogMyikaMCPChat, Log, TEXT("MyikaMCPChat module shutting down."));
	UnregisterTabSpawner();
	Backend.Reset();
}

TSharedPtr<IMyikaChatBackend> FMyikaMCPChatModule::GetOrCreateBackend()
{
	if (Backend.IsValid())
	{
		return Backend;
	}
	const UMyikaMCPChatSettings* S = GetDefault<UMyikaMCPChatSettings>();
	const EMyikaChatBackendKind Kind = S ? S->Backend : EMyikaChatBackendKind::ClaudeCode;
	switch (Kind)
	{
	case EMyikaChatBackendKind::AnthropicDirect:
		Backend = MakeShared<FAnthropicDirectBackend>();
		break;
	case EMyikaChatBackendKind::ClaudeCode:
	default:
		Backend = MakeShared<FClaudeCodeBackend>();
		break;
	}
	UE_LOG(LogMyikaMCPChat, Log, TEXT("Backend created: %s"), *Backend->GetDisplayName());
	return Backend;
}

void FMyikaMCPChatModule::RebuildBackend()
{
	Backend.Reset();
	GetOrCreateBackend();
}

void FMyikaMCPChatModule::OpenChatTab()
{
	FGlobalTabmanager::Get()->TryInvokeTab(ChatTabId);
}

void FMyikaMCPChatModule::RegisterTabSpawner()
{
	FGlobalTabmanager::Get()->RegisterNomadTabSpawner(
			ChatTabId,
			FOnSpawnTab::CreateRaw(this, &FMyikaMCPChatModule::SpawnChatTab))
		.SetDisplayName(FText::FromString(TEXT("Myika Chat")))
		.SetTooltipText(FText::FromString(TEXT("In-editor chat with Myika.")))
		.SetGroup(WorkspaceMenu::GetMenuStructure().GetToolsCategory());
}

void FMyikaMCPChatModule::UnregisterTabSpawner()
{
	// FGlobalTabmanager::Get() returns a TSharedRef — no null check needed/possible.
	FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(ChatTabId);
}

TSharedRef<SDockTab> FMyikaMCPChatModule::SpawnChatTab(const FSpawnTabArgs& /*Args*/)
{
	return SNew(SDockTab)
		.TabRole(ETabRole::NomadTab)
		[
			SNew(SMyikaChatPanel).Backend(GetOrCreateBackend())
		];
}

void FMyikaMCPChatModule::RegisterToolbar()
{
	UToolMenus* Menus = UToolMenus::Get();
	UToolMenu* ToolBar = Menus->ExtendMenu(TEXT("LevelEditor.LevelEditorToolBar.PlayToolBar"));
	if (!ToolBar)
	{
		UE_LOG(LogMyikaMCPChat, Warning, TEXT("Could not find LevelEditor PlayToolBar; chat toolbar entry will not appear."));
		return;
	}

	FToolMenuSection& Section = ToolBar->FindOrAddSection(TEXT("MyikaMCPChat"));

	// "Open Myika Chat" — primary entry.
	Section.AddEntry(FToolMenuEntry::InitToolBarButton(
		TEXT("MyikaChatOpen"),
		FUIAction(FExecuteAction::CreateLambda([]()
		{
			FMyikaMCPChatModule::Get().OpenChatTab();
		})),
		FText::FromString(TEXT("Myika Chat")),
		FText::FromString(TEXT("Open the in-editor Myika chat panel.")),
		FSlateIcon()
	));

	// "New Chat" — adjacent quick action that resets conversation state. Per spec.
	Section.AddEntry(FToolMenuEntry::InitToolBarButton(
		TEXT("MyikaChatNew"),
		FUIAction(FExecuteAction::CreateLambda([]()
		{
			FMyikaMCPChatModule::Get().OpenChatTab();
			if (TSharedPtr<IMyikaChatBackend> B = FMyikaMCPChatModule::Get().GetOrCreateBackend())
			{
				B->NewChat();
			}
		})),
		FText::FromString(TEXT("New Chat")),
		FText::FromString(TEXT("Wipe the current Myika chat conversation and start fresh.")),
		FSlateIcon()
	));
}
