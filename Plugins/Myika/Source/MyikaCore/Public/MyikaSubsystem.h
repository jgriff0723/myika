// Copyright (c) Myika AI. All rights reserved.
//
// UMyikaSubsystem
//
// Process-wide service locator + global state broadcaster for the Myika plugin family.
// Owns the canonical FMyikaGlobalState. Per-system actors (BP_MyikaSky, BP_MyikaWeather,
// BP_MyikaWaterController, etc.) read from this subsystem and write back through it,
// instead of finding each other by direct reference.
//
// MPC sync is performed by a thin component (UMyikaMPCSyncComponent, added in Phase 1)
// that listens to OnGlobalStateChanged and pushes scalars into MPC_Myika so any material
// can sample them. The subsystem itself never touches the MPC directly — keeps
// MyikaCore free of material/render dependencies.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "MyikaTypes.h"
#include "MyikaSubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FMyikaGlobalStateChanged, const FMyikaGlobalState&, NewState);

UCLASS(DisplayName = "Myika Subsystem")
class MYIKACORE_API UMyikaSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	/** Read the current global state. */
	UFUNCTION(BlueprintPure, Category = "Myika")
	const FMyikaGlobalState& GetGlobalState() const { return GlobalState; }

	/** Replace the entire global state. Broadcasts OnGlobalStateChanged if anything differs. */
	UFUNCTION(BlueprintCallable, Category = "Myika")
	void SetGlobalState(const FMyikaGlobalState& NewState);

	/** Patch a single time-of-day value; convenience for sky systems. */
	UFUNCTION(BlueprintCallable, Category = "Myika|Sky")
	void SetTimeOfDay(float HoursZeroToTwentyFour);

	/** Patch a single sun-direction vector without replacing the rest of the global state. */
	UFUNCTION(BlueprintCallable, Category = "Myika|Sky|Sun")
	void SetSunDirection(FVector NewSunDirection);

	/** Patch a single moon-direction vector without replacing the rest of the global state. */
	UFUNCTION(BlueprintCallable, Category = "Myika|Sky|Stars")
	void SetMoonDirection(FVector NewMoonDirection);

	/** Patch a single weather state; convenience for weather systems. */
	UFUNCTION(BlueprintCallable, Category = "Myika|Weather")
	void SetWeather(EMyikaWeatherState NewWeather);

	/** Patch storm intensity 0..1; convenience for sky and weather systems. */
	UFUNCTION(BlueprintCallable, Category = "Myika|Sky")
	void SetStormIntensity(float NewStormIntensity);

	/** Patch lightning flash 0..1; convenience for lightning and material reactions. */
	UFUNCTION(BlueprintCallable, Category = "Myika|Sky|Lightning")
	void SetLightningFlash(float NewLightningFlash);

	/** Broadcast whenever any global field changes. */
	UPROPERTY(BlueprintAssignable, Category = "Myika")
	FMyikaGlobalStateChanged OnGlobalStateChanged;

private:
	UPROPERTY()
	FMyikaGlobalState GlobalState;

	void BroadcastIfChanged(const FMyikaGlobalState& Previous);
};
