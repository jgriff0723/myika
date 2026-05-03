// Copyright (c) Myika AI. All rights reserved.
//
// Shared types for the Myika plugin family. Per-system modules (MyikaSky, MyikaWater, etc.)
// depend on this header to share enums/structs across the plugin.

#pragma once

#include "CoreMinimal.h"
#include "MyikaTypes.generated.h"

/** Time-of-day part used by sky/weather/NPC routine systems. */
UENUM(BlueprintType)
enum class EMyikaPartOfDay : uint8
{
	Dawn        UMETA(DisplayName = "Dawn"),
	Morning     UMETA(DisplayName = "Morning"),
	Noon        UMETA(DisplayName = "Noon"),
	Afternoon   UMETA(DisplayName = "Afternoon"),
	Dusk        UMETA(DisplayName = "Dusk"),
	Night       UMETA(DisplayName = "Night")
};

/** Coarse weather state shared across systems (full state machine lives in MyikaWeather later). */
UENUM(BlueprintType)
enum class EMyikaWeatherState : uint8
{
	Clear         UMETA(DisplayName = "Clear"),
	PartlyCloudy  UMETA(DisplayName = "Partly Cloudy"),
	Overcast      UMETA(DisplayName = "Overcast"),
	LightRain     UMETA(DisplayName = "Light Rain"),
	HeavyRain     UMETA(DisplayName = "Heavy Rain"),
	Thunderstorm  UMETA(DisplayName = "Thunderstorm"),
	Snow          UMETA(DisplayName = "Snow"),
	Blizzard      UMETA(DisplayName = "Blizzard"),
	Foggy         UMETA(DisplayName = "Foggy"),
	SandStorm     UMETA(DisplayName = "Sand Storm")
};

/** Snapshot of myika global state, broadcast on change via the subsystem. */
USTRUCT(BlueprintType)
struct MYIKACORE_API FMyikaGlobalState
{
	GENERATED_BODY()

	/** 0..24 game-time hours. */
	UPROPERTY(BlueprintReadOnly, Category = "Myika")
	float TimeOfDay = 12.f;

	UPROPERTY(BlueprintReadOnly, Category = "Myika")
	EMyikaPartOfDay PartOfDay = EMyikaPartOfDay::Noon;

	UPROPERTY(BlueprintReadOnly, Category = "Myika")
	EMyikaWeatherState Weather = EMyikaWeatherState::Clear;

	/** Unit-direction vector pointing FROM the sun (i.e. sunlight direction). */
	UPROPERTY(BlueprintReadOnly, Category = "Myika")
	FVector SunDirection = FVector(0.f, 0.f, -1.f);

	/** Unit-direction vector pointing FROM the moon (i.e. moonlight direction). */
	UPROPERTY(BlueprintReadOnly, Category = "Myika")
	FVector MoonDirection = FVector(0.f, 0.f, 1.f);

	/** Global wind vector in cm/s. */
	UPROPERTY(BlueprintReadOnly, Category = "Myika")
	FVector WindVector = FVector::ZeroVector;

	/** 0..1 surface wetness (drives material macros). */
	UPROPERTY(BlueprintReadOnly, Category = "Myika")
	float Wetness = 0.f;

	/** 0..1 snow coverage (drives material macros). */
	UPROPERTY(BlueprintReadOnly, Category = "Myika")
	float SnowCoverage = 0.f;

	/** Ambient temperature in degrees C. */
	UPROPERTY(BlueprintReadOnly, Category = "Myika")
	float Temperature = 20.f;

	/** 0..1 instantaneous lightning flash intensity. */
	UPROPERTY(BlueprintReadOnly, Category = "Myika")
	float LightningFlash = 0.f;

	/** Storm intensity 0..1 used by sky/cloud morphology. */
	UPROPERTY(BlueprintReadOnly, Category = "Myika")
	float StormIntensity = 0.f;

	/** Global water plane height in cm. */
	UPROPERTY(BlueprintReadOnly, Category = "Myika")
	float WaterLevel = 0.f;
};
