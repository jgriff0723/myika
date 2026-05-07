// Copyright (c) Myika AI. All rights reserved.

#include "MyikaSkyEditorModule.h"

DEFINE_LOG_CATEGORY_STATIC(LogMyikaSkyEditor, Log, All);

void FMyikaSkyEditorModule::StartupModule()
{
	UE_LOG(LogMyikaSkyEditor, Log, TEXT("MyikaSkyEditor module started."));
}

void FMyikaSkyEditorModule::ShutdownModule()
{
	UE_LOG(LogMyikaSkyEditor, Log, TEXT("MyikaSkyEditor module shutdown."));
}

IMPLEMENT_MODULE(FMyikaSkyEditorModule, MyikaSkyEditor)
