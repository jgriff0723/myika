// Copyright (c) Myika AI. All rights reserved.

#include "MyikaMCPToolbar.h"
#include "MyikaMCPLog.h"
#include "MyikaMCPModule.h"
#include "MyikaMCPSettings.h"
#include "Framework/Commands/UICommandList.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "ISettingsModule.h"
#include "Modules/ModuleManager.h"
#include "ToolMenus.h"

void FMyikaMCPToolbarExtension::Register()
{
	if (bRegistered || !UToolMenus::IsToolMenuUIEnabled())
	{
		return;
	}
	bRegistered = true;

	CommandList = MakeShared<FUICommandList>();

	UToolMenus* Menus = UToolMenus::Get();
	UToolMenu* ToolBar = Menus->ExtendMenu(TEXT("LevelEditor.LevelEditorToolBar.PlayToolBar"));
	if (!ToolBar)
	{
		UE_LOG(LogMyikaMCP, Warning, TEXT("Could not find LevelEditor PlayToolBar; toolbar entry will not appear."));
		return;
	}

	FToolMenuSection& Section = ToolBar->FindOrAddSection(TEXT("MyikaMCP"));

	TWeakPtr<FMyikaMCPToolbarExtension> WeakSelf = AsShared();
	FToolMenuEntry Entry = FToolMenuEntry::InitComboButton(
		TEXT("MyikaMCPCombo"),
		FUIAction(),
		FNewToolMenuChoice(FNewToolMenuDelegate::CreateLambda([WeakSelf](UToolMenu* SubMenu)
		{
			TSharedPtr<FMyikaMCPToolbarExtension> Pinned = WeakSelf.Pin();
			if (!Pinned.IsValid()) { return; }

			FToolMenuSection& S = SubMenu->AddSection(TEXT("MyikaMCPActions"), FText::FromString(TEXT("Myika MCP")));
			S.AddMenuEntry(
				TEXT("Start"),
				FText::FromString(TEXT("Start Server")),
				FText::FromString(TEXT("Bind 127.0.0.1 on the configured port and accept MCP traffic.")),
				FSlateIcon(),
				FUIAction(
					FExecuteAction::CreateSP(Pinned.ToSharedRef(), &FMyikaMCPToolbarExtension::OnStart),
					FCanExecuteAction::CreateLambda([Pinned]() { return !Pinned->IsRunning(); })
				));
			S.AddMenuEntry(
				TEXT("Stop"),
				FText::FromString(TEXT("Stop Server")),
				FText::FromString(TEXT("Shut down the listener and close all connections.")),
				FSlateIcon(),
				FUIAction(
					FExecuteAction::CreateSP(Pinned.ToSharedRef(), &FMyikaMCPToolbarExtension::OnStop),
					FCanExecuteAction::CreateLambda([Pinned]() { return Pinned->IsRunning(); })
				));
			S.AddMenuEntry(
				TEXT("Restart"),
				FText::FromString(TEXT("Restart Server")),
				FText::FromString(TEXT("Stop then start again. Useful after a port change.")),
				FSlateIcon(),
				FUIAction(FExecuteAction::CreateSP(Pinned.ToSharedRef(), &FMyikaMCPToolbarExtension::OnRestart)));
			S.AddSeparator(TEXT("MyikaMCPSep"));
			S.AddMenuEntry(
				TEXT("Settings"),
				FText::FromString(TEXT("Open Settings…")),
				FText::FromString(TEXT("Open Project Settings for Myika MCP.")),
				FSlateIcon(),
				FUIAction(FExecuteAction::CreateSP(Pinned.ToSharedRef(), &FMyikaMCPToolbarExtension::OnOpenSettings)));
		})),
		TAttribute<FText>::CreateLambda([WeakSelf]() -> FText
		{
			TSharedPtr<FMyikaMCPToolbarExtension> Pinned = WeakSelf.Pin();
			if (Pinned.IsValid() && Pinned->IsRunning())
			{
				return FText::FromString(TEXT("Myika MCP ●"));
			}
			return FText::FromString(TEXT("Myika MCP ○"));
		}),
		TAttribute<FText>::CreateLambda([]() -> FText
		{
			return FText::FromString(FMyikaMCPModule::Get().GetStatusString());
		}),
		FSlateIcon()
	);
	Section.AddEntry(Entry);
}

void FMyikaMCPToolbarExtension::Unregister()
{
	if (!bRegistered) { return; }
	bRegistered = false;
	if (UToolMenus* Menus = UToolMenus::Get())
	{
		if (UToolMenu* ToolBar = Menus->FindMenu(TEXT("LevelEditor.LevelEditorToolBar.PlayToolBar")))
		{
			ToolBar->RemoveSection(TEXT("MyikaMCP"));
		}
	}
	CommandList.Reset();
}

void FMyikaMCPToolbarExtension::OnStart()
{
	FMyikaMCPModule::Get().StartServer();
}

void FMyikaMCPToolbarExtension::OnStop()
{
	FMyikaMCPModule::Get().StopServer();
}

void FMyikaMCPToolbarExtension::OnRestart()
{
	FMyikaMCPModule::Get().StopServer();
	FMyikaMCPModule::Get().StartServer();
}

void FMyikaMCPToolbarExtension::OnOpenSettings()
{
	if (ISettingsModule* Settings = FModuleManager::GetModulePtr<ISettingsModule>("Settings"))
	{
		Settings->ShowViewer(TEXT("Project"), TEXT("Plugins"), TEXT("Myika MCP"));
	}
}

bool FMyikaMCPToolbarExtension::IsRunning() const
{
	return FMyikaMCPModule::Get().IsServerRunning();
}
