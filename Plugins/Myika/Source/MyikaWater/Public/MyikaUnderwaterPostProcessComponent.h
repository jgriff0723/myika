// Copyright (c) Myika AI. All rights reserved.

#pragma once

#include "Components/ActorComponent.h"
#include "MyikaUnderwaterPostProcessComponent.generated.h"

class UMaterialInstanceDynamic;
class UMaterialInterface;
class UPostProcessComponent;

UCLASS(ClassGroup = (Myika), meta = (BlueprintSpawnableComponent), DisplayName = "Myika Underwater Post Process")
class MYIKAWATER_API UMyikaUnderwaterPostProcessComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UMyikaUnderwaterPostProcessComponent();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/** Returns whether the underwater effect is currently active for the tracked player camera. */
	UFUNCTION(BlueprintPure, Category = "Myika|Water|Underwater")
	bool IsUnderwaterActive() const { return bEffectActive; }

	/** Returns the most recent camera depth under the queried water surface in centimeters. */
	UFUNCTION(BlueprintPure, Category = "Myika|Water|Underwater")
	float GetCurrentDepthBelowSurfaceCm() const { return CurrentDepthBelowSurfaceCm; }

	/** Post-process material applied when the tracked player camera is underwater. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Myika|Water|Underwater")
	TObjectPtr<UMaterialInterface> UnderwaterPostProcessMaterial = nullptr;

	/** Water fog tint sent to the underwater post-process material. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Myika|Water|Underwater")
	FLinearColor WaterColor = FLinearColor(0.035f, 0.22f, 0.32f, 1.0f);

	/** Fog density in 1/m used by the underwater post-process material. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Myika|Water|Underwater",
		meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "0.25", SliderExponent = "2.0"))
	float FogDensity = 0.035f;

	/** Screen-space refraction distortion strength used by the underwater post-process material. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Myika|Water|Underwater",
		meta = (ClampMin = "0.0", ClampMax = "0.25", UIMin = "0.0", UIMax = "0.1", SliderExponent = "2.0"))
	float RefractionStrength = 0.02f;

	/** Surface-band smoothing used by the waterline gradient in the underwater post-process material. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Myika|Water|Underwater",
		meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0", SliderExponent = "2.0"))
	float WaterlineSmoothness = 0.2f;

	/** Priority for the internally owned unbound post-process component. Lower values stay out of the way of cinematic volumes. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Myika|Water|Underwater",
		meta = (UIMin = "-100.0", UIMax = "100.0"))
	float PostProcessPriority = -1.0f;

	/** Extra depth below the surface required before the effect toggles on, to reduce waterline chatter. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Myika|Water|Underwater",
		meta = (ClampMin = "0.0", ClampMax = "100.0", UIMin = "0.0", UIMax = "50.0", Units = "cm"))
	float ActivationDepthBiasCm = 4.0f;

	/** Includes water-wave displacement in the surface query when supported by the current water body. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Myika|Water|Underwater")
	bool bIncludeWaves = true;

	/** Whether the underwater effect is currently active. */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Myika|Debug")
	bool bEffectActive = false;

	/** Most recent depth below the queried water surface in centimeters. */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Myika|Debug",
		meta = (Units = "cm"))
	float CurrentDepthBelowSurfaceCm = 0.0f;

	/** Most recent queried water-surface Z in world space. */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Myika|Debug",
		meta = (Units = "cm"))
	float CurrentWaterSurfaceZ = 0.0f;

private:
	void EnsurePostProcessComponent();
	void RefreshMaterialInstance();
	void PushMaterialParameters();
	bool ResolvePlayerViewLocation(FVector& OutViewLocation) const;
	bool ResolveNearestWaterSurface(const FVector& ViewLocation, float& OutSurfaceZ, float& OutDepthBelowSurfaceCm) const;
	void ApplyEffectState(bool bShouldEnable, float SurfaceZ, float DepthBelowSurfaceCm);

	UPROPERTY(Transient)
	TObjectPtr<UPostProcessComponent> PostProcessComponent;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> PostProcessMaterialInstance;
};
