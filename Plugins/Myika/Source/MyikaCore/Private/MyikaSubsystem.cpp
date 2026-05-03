// Copyright (c) Myika AI. All rights reserved.

#include "MyikaSubsystem.h"

DEFINE_LOG_CATEGORY_STATIC(LogMyikaSubsystem, Log, All);

namespace
{
	EMyikaPartOfDay TimeOfDayToPart(float Hours)
	{
		if (Hours < 5.0f)  { return EMyikaPartOfDay::Night; }
		if (Hours < 7.0f)  { return EMyikaPartOfDay::Dawn; }
		if (Hours < 11.0f) { return EMyikaPartOfDay::Morning; }
		if (Hours < 13.0f) { return EMyikaPartOfDay::Noon; }
		if (Hours < 17.0f) { return EMyikaPartOfDay::Afternoon; }
		if (Hours < 20.0f) { return EMyikaPartOfDay::Dusk; }
		return EMyikaPartOfDay::Night;
	}
}

void UMyikaSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	UE_LOG(LogMyikaSubsystem, Log, TEXT("MyikaSubsystem initialized."));
}

void UMyikaSubsystem::Deinitialize()
{
	UE_LOG(LogMyikaSubsystem, Log, TEXT("MyikaSubsystem deinitialized."));
	Super::Deinitialize();
}

void UMyikaSubsystem::SetGlobalState(const FMyikaGlobalState& NewState)
{
	const FMyikaGlobalState Previous = GlobalState;
	GlobalState = NewState;
	BroadcastIfChanged(Previous);
}

void UMyikaSubsystem::SetTimeOfDay(float HoursZeroToTwentyFour)
{
	const FMyikaGlobalState Previous = GlobalState;
	GlobalState.TimeOfDay = FMath::Fmod(FMath::Max(0.f, HoursZeroToTwentyFour), 24.0f);
	GlobalState.PartOfDay = TimeOfDayToPart(GlobalState.TimeOfDay);
	BroadcastIfChanged(Previous);
}

void UMyikaSubsystem::SetWeather(EMyikaWeatherState NewWeather)
{
	const FMyikaGlobalState Previous = GlobalState;
	GlobalState.Weather = NewWeather;
	BroadcastIfChanged(Previous);
}

void UMyikaSubsystem::BroadcastIfChanged(const FMyikaGlobalState& Previous)
{
	const bool bChanged =
		!FMath::IsNearlyEqual(Previous.TimeOfDay, GlobalState.TimeOfDay) ||
		Previous.PartOfDay != GlobalState.PartOfDay ||
		Previous.Weather != GlobalState.Weather ||
		!Previous.SunDirection.Equals(GlobalState.SunDirection) ||
		!Previous.MoonDirection.Equals(GlobalState.MoonDirection) ||
		!Previous.WindVector.Equals(GlobalState.WindVector) ||
		!FMath::IsNearlyEqual(Previous.Wetness, GlobalState.Wetness) ||
		!FMath::IsNearlyEqual(Previous.SnowCoverage, GlobalState.SnowCoverage) ||
		!FMath::IsNearlyEqual(Previous.Temperature, GlobalState.Temperature) ||
		!FMath::IsNearlyEqual(Previous.StormIntensity, GlobalState.StormIntensity);

	if (bChanged)
	{
		OnGlobalStateChanged.Broadcast(GlobalState);
	}
}
