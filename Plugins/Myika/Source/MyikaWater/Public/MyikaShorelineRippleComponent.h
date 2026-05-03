// Copyright (c) Myika AI. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "UObject/SoftObjectPtr.h"
#include "MyikaShorelineRippleComponent.generated.h"

class AWaterBody;
class UTextureRenderTarget2D;
class UNiagaraComponent;
class UNiagaraSystem;

UCLASS(
	BlueprintType,
	Blueprintable,
	ClassGroup = (Myika),
	meta = (BlueprintSpawnableComponent, DisplayName = "Myika Shoreline Ripple Component"))
class MYIKAWATER_API UMyikaShorelineRippleComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UMyikaShorelineRippleComponent();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/** Force an interaction stamp at a world-space location. */
	UFUNCTION(BlueprintCallable, Category = "Myika|Water|Ripples")
	void InjectRippleAtWorldLocation(FVector WorldLocation, float RadiusCm = -1.0f, float Strength = -1.0f);

	/** Override the water body used for water-surface queries. */
	UFUNCTION(BlueprintCallable, Category = "Myika|Water|Ripples")
	void SetTrackedWaterBody(AWaterBody* InWaterBody);

	/** Read the water body currently used for water-surface queries. */
	UFUNCTION(BlueprintPure, Category = "Myika|Water|Ripples")
	AWaterBody* GetTrackedWaterBody() const { return TrackedWaterBody.Get(); }

	/** Read the output normal render target currently bound to Niagara. */
	UFUNCTION(BlueprintPure, Category = "Myika|Water|Ripples")
	UTextureRenderTarget2D* GetRippleNormalRenderTarget() const { return RippleNormalRenderTarget; }

	/** Read the current camera-anchored simulation center in world space. */
	UFUNCTION(BlueprintPure, Category = "Myika|Water|Ripples")
	FVector GetCurrentDomainCenter() const { return CurrentDomainCenter; }

protected:
	/** Niagara system duplicated from the Grid2D Shallow Water template for shoreline ripples. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Myika|Water|Ripples|Assets")
	TObjectPtr<UNiagaraSystem> RippleSystem = nullptr;

	/** Normal-map render target exported by the shoreline ripple Niagara system. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Myika|Water|Ripples|Assets")
	TObjectPtr<UTextureRenderTarget2D> RippleNormalRenderTarget = nullptr;

	/** Canonical Niagara asset loaded when the live component leaves RippleSystem unset. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Myika|Water|Ripples|Assets",
		meta = (AllowedClasses = "/Script/Niagara.NiagaraSystem"))
	TSoftObjectPtr<UNiagaraSystem> DefaultRippleSystemAsset;

	/** Canonical render target loaded when the live component leaves RippleNormalRenderTarget unset. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Myika|Water|Ripples|Assets",
		meta = (AllowedClasses = "/Script/Engine.TextureRenderTarget2D"))
	TSoftObjectPtr<UTextureRenderTarget2D> DefaultRippleNormalRenderTargetAsset;

	/** When enabled, the component chooses the nearest overlapping water body automatically. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Myika|Water|Ripples|Tracking")
	bool bAutoTrackWaterBody = true;

	/** When enabled, owner motion periodically injects ripples without extra Blueprint wiring. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Myika|Water|Ripples|Tracking")
	bool bAutoInjectFromOwnerMotion = true;

	/** Pause the Niagara sim whenever the owner is not in water overlap range. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Myika|Water|Ripples|Performance")
	bool bOnlySimulateNearWater = true;

	/** Radius of the camera-following ripple simulation domain in centimeters. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Myika|Water|Ripples|Domain",
		meta = (ClampMin = "500.0", ClampMax = "12000.0", UIMin = "500.0", UIMax = "6000.0"))
	float SimulationRadiusCm = 3000.0f;

	/** Maximum vertical gap from the water surface that still counts as a shoreline interaction. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Myika|Water|Ripples|Tracking",
		meta = (ClampMin = "0.0", ClampMax = "250.0", UIMin = "0.0", UIMax = "100.0"))
	float MaxInteractionHeightAboveSurfaceCm = 25.0f;

	/** Minimum owner speed before auto-injected ripples start spawning. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Myika|Water|Ripples|Tracking",
		meta = (ClampMin = "0.0", ClampMax = "1200.0", UIMin = "0.0", UIMax = "600.0"))
	float MinimumInjectionSpeedCmPerSecond = 35.0f;

	/** Minimum time between auto-injected ripple stamps. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Myika|Water|Ripples|Tracking",
		meta = (ClampMin = "0.01", ClampMax = "1.0", UIMin = "0.01", UIMax = "0.25"))
	float InjectionIntervalSeconds = 0.08f;

	/** Default ripple radius written into the Niagara system when no override is provided. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Myika|Water|Ripples|Injection",
		meta = (ClampMin = "1.0", ClampMax = "500.0", UIMin = "10.0", UIMax = "200.0"))
	float DefaultRippleRadiusCm = 55.0f;

	/** Default ripple strength written into the Niagara system when no override is provided. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Myika|Water|Ripples|Injection",
		meta = (ClampMin = "0.0", ClampMax = "10.0", UIMin = "0.0", UIMax = "3.0", SliderExponent = "2.0"))
	float DefaultRippleStrength = 1.0f;

	/** Socket names sampled on the owner skeletal mesh for foot and hand interactions. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Myika|Water|Ripples|Tracking")
	TArray<FName> InteractionSocketNames;

	/** Niagara user parameter bound to the output normal render target. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Myika|Water|Ripples|Niagara")
	FName RippleNormalRenderTargetParameterName = TEXT("RipplesNormalRT");

	/** Niagara user parameter that receives the camera-anchored simulation center. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Myika|Water|Ripples|Niagara")
	FName DomainCenterParameterName = TEXT("DomainCenterWS");

	/** Niagara user parameter that receives the simulation radius in centimeters. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Myika|Water|Ripples|Niagara")
	FName DomainRadiusParameterName = TEXT("DomainRadiusCm");

	/** Niagara user parameter that receives the current ripple injection point. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Myika|Water|Ripples|Niagara")
	FName InjectionPointParameterName = TEXT("RipplePointWS");

	/** Niagara user parameter that receives the current ripple radius. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Myika|Water|Ripples|Niagara")
	FName InjectionRadiusParameterName = TEXT("RippleRadiusCm");

	/** Niagara user parameter that receives the current ripple strength. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Myika|Water|Ripples|Niagara")
	FName InjectionStrengthParameterName = TEXT("RippleStrength");

	/** Niagara user parameter incremented on every injection so the system can detect new stamps. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Myika|Water|Ripples|Niagara")
	FName InjectionSequenceParameterName = TEXT("RippleSequence");

	/** Niagara user parameter toggled when shoreline simulation should actively run. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Myika|Water|Ripples|Niagara")
	FName SimulationEnabledParameterName = TEXT("SimulationEnabled");

private:
	void ResolveDefaultAssets();
	void EnsureNiagaraComponent();
	void PushNiagaraBindings() const;
	void RefreshTrackedWaterBody();
	void UpdateSimulationState(float DeltaTime);
	void ApplyNiagaraActivation(bool bShouldBeActive);
	FVector ResolveDomainCenter() const;
	TArray<FVector> CollectInteractionPoints() const;
	bool QueryWaterAtLocation(const FVector& WorldLocation, FVector& OutWaterSurfaceLocation, float& OutWaterDepth) const;

	UPROPERTY(Transient)
	TObjectPtr<UNiagaraComponent> NiagaraComponent = nullptr;

	TWeakObjectPtr<AWaterBody> TrackedWaterBody;
	FVector CurrentDomainCenter = FVector::ZeroVector;
	float TimeUntilNextInjection = 0.0f;
	float RippleSequence = 0.0f;
	bool bNiagaraActive = false;
};
