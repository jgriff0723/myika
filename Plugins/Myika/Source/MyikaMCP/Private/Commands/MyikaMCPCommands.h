// Copyright (c) Myika AI. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "MyikaMCPCommandRegistry.h"

namespace MyikaMCP::Commands
{
	void RegisterAll();
	void UnregisterAll();

	// Factory functions — one per Cmd_*.cpp file. Each returns an FMyikaMCPHandlerFn.
	FMyikaMCPHandlerFn MakePingHandler();
	FMyikaMCPHandlerFn MakeGetGlobalStateHandler();
	FMyikaMCPHandlerFn MakeSetGlobalStateHandler();
	FMyikaMCPHandlerFn MakeSetTimeOfDayHandler();
	FMyikaMCPHandlerFn MakeSetWeatherHandler();
	FMyikaMCPHandlerFn MakeSetStormIntensityHandler();
	FMyikaMCPHandlerFn MakeSetWindHandler();
	FMyikaMCPHandlerFn MakeSetWetnessHandler();
	FMyikaMCPHandlerFn MakeSetSnowCoverageHandler();
	FMyikaMCPHandlerFn MakeSetTemperatureHandler();
	FMyikaMCPHandlerFn MakePhase2StubHandler(const FString& FeatureName);
}
