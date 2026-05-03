// Copyright (c) Myika AI. All rights reserved.

#include "MyikaCoreModule.h"

DEFINE_LOG_CATEGORY_STATIC(LogMyikaCore, Log, All);

void FMyikaCoreModule::StartupModule()
{
	UE_LOG(LogMyikaCore, Log, TEXT("MyikaCore module started."));
}

void FMyikaCoreModule::ShutdownModule()
{
	UE_LOG(LogMyikaCore, Log, TEXT("MyikaCore module shutdown."));
}

IMPLEMENT_MODULE(FMyikaCoreModule, MyikaCore)
