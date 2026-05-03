// Copyright (c) Myika AI. All rights reserved.

#include "MyikaWaterModule.h"

DEFINE_LOG_CATEGORY_STATIC(LogMyikaWater, Log, All);

void FMyikaWaterModule::StartupModule()
{
	UE_LOG(LogMyikaWater, Log, TEXT("MyikaWater module started."));
}

void FMyikaWaterModule::ShutdownModule()
{
	UE_LOG(LogMyikaWater, Log, TEXT("MyikaWater module shutdown."));
}

IMPLEMENT_MODULE(FMyikaWaterModule, MyikaWater)
