// Copyright (c) Myika AI. All rights reserved.

#include "MyikaSubsystem.h"

DEFINE_LOG_CATEGORY_STATIC(LogMyikaSubsystem, Log, All);

namespace
{
	FVector NormalizeDirectionOrFallback(const FVector& Direction, const FVector& Fallback)
	{
		return Direction.IsNearlyZero() ? Fallback : Direction.GetSafeNormal();
	}

	float NormalizeTimeOfDay(const float Hours)
	{
		const float Wrapped = FMath::Fmod(FMath::Max(0.0f, Hours), 24.0f);
		return Wrapped < 0.0f ? Wrapped + 24.0f : Wrapped;
	}

	float ClampUnit(const float Value)
	{
		return FMath::Clamp(Value, 0.0f, 1.0f);
	}

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
	GlobalState.TimeOfDay = NormalizeTimeOfDay(GlobalState.TimeOfDay);
	GlobalState.PartOfDay = TimeOfDayToPart(GlobalState.TimeOfDay);
	GlobalState.Wetness = ClampUnit(GlobalState.Wetness);
	GlobalState.SnowCoverage = ClampUnit(GlobalState.SnowCoverage);
	GlobalState.LightningFlash = ClampUnit(GlobalState.LightningFlash);
	GlobalState.StormIntensity = ClampUnit(GlobalState.StormIntensity);
	GlobalState.SunDirection = NormalizeDirectionOrFallback(GlobalState.SunDirection, FVector::DownVector);
	GlobalState.MoonDirection = NormalizeDirectionOrFallback(GlobalState.MoonDirection, FVector::UpVector);
	BroadcastIfChanged(Previous);
}

void UMyikaSubsystem::SetTimeOfDay(float HoursZeroToTwentyFour)
{
	const FMyikaGlobalState Previous = GlobalState;
	GlobalState.TimeOfDay = NormalizeTimeOfDay(HoursZeroToTwentyFour);
	GlobalState.PartOfDay = TimeOfDayToPart(GlobalState.TimeOfDay);
	BroadcastIfChanged(Previous);
}

void UMyikaSubsystem::SetSunDirection(FVector NewSunDirection)
{
	const FMyikaGlobalState Previous = GlobalState;
	GlobalState.SunDirection = NormalizeDirectionOrFallback(NewSunDirection, FVector::DownVector);
	BroadcastIfChanged(Previous);
}

void UMyikaSubsystem::SetMoonDirection(FVector NewMoonDirection)
{
	const FMyikaGlobalState Previous = GlobalState;
	GlobalState.MoonDirection = NormalizeDirectionOrFallback(NewMoonDirection, FVector::UpVector);
	BroadcastIfChanged(Previous);
}

void UMyikaSubsystem::SetWeather(EMyikaWeatherState NewWeather)
{
	const FMyikaGlobalState Previous = GlobalState;
	GlobalState.Weather = NewWeather;
	BroadcastIfChanged(Previous);
}

void UMyikaSubsystem::SetStormIntensity(float NewStormIntensity)
{
	const FMyikaGlobalState Previous = GlobalState;
	GlobalState.StormIntensity = ClampUnit(NewStormIntensity);
	BroadcastIfChanged(Previous);
}

void UMyikaSubsystem::SetLightningFlash(float NewLightningFlash)
{
	const FMyikaGlobalState Previous = GlobalState;
	GlobalState.LightningFlash = ClampUnit(NewLightningFlash);
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
		!FMath::IsNearlyEqual(Previous.LightningFlash, GlobalState.LightningFlash) ||
		!FMath::IsNearlyEqual(Previous.StormIntensity, GlobalState.StormIntensity) ||
		!FMath::IsNearlyEqual(Previous.WaterLevel, GlobalState.WaterLevel);

	if (bChanged)
	{
		OnGlobalStateChanged.Broadcast(GlobalState);
	}
}
