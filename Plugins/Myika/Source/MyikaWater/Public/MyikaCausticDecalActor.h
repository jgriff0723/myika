// Copyright (c) Myika AI. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DecalActor.h"
#include "MyikaCausticDecalActor.generated.h"

class UMaterialInstanceDynamic;
class UMaterialInterface;
struct FPropertyChangedEvent;

UCLASS(BlueprintType, Blueprintable, ClassGroup = (Myika), meta = (DisplayName = "Myika Caustic Decal"))
class MYIKAWATER_API AMyikaCausticDecalActor : public ADecalActor
{
	GENERATED_BODY()

public:
	AMyikaCausticDecalActor();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

	/** Update the projected decal half-extents in world space. */
	UFUNCTION(BlueprintCallable, Category = "Myika|Water|Caustics")
	void SetProjectionExtent(const FVector& NewExtent);

	/** Override the fallback sun direction used before MPC/state sync is live. */
	UFUNCTION(BlueprintCallable, Category = "Myika|Water|Caustics")
	void SetFallbackSunDirection(const FVector& NewSunDirection);

protected:
	/** Deferred decal material that samples the caustic flipbook atlas. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Myika|Water|Caustics")
	TObjectPtr<UMaterialInterface> CausticMaterial;

	/** Half-extents of the underwater projection box. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Myika|Water|Caustics")
	FVector ProjectionExtent = FVector(4096.f, 4096.f, 2048.f);

	/** Frames advanced per second while cycling the caustic atlas. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Myika|Water|Caustics",
		meta = (ClampMin = "0.0", ClampMax = "32.0", UIMin = "0.0", UIMax = "16.0", SliderExponent = "2.0"))
	float AnimationSpeed = 1.5f;

	/** Overall brightness multiplier for the projected caustics. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Myika|Water|Caustics",
		meta = (ClampMin = "0.0", ClampMax = "10.0", UIMin = "0.0", UIMax = "5.0", SliderExponent = "2.0"))
	float CausticIntensity = 1.0f;

	/** How strongly the sun direction skews the projected pattern. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Myika|Water|Caustics",
		meta = (ClampMin = "0.0", ClampMax = "4.0", UIMin = "0.0", UIMax = "2.0", SliderExponent = "2.0"))
	float SunDriftStrength = 1.0f;

	/** Fallback direction pointing from the sun when MPC-driven sync is not available yet. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Myika|Water|Caustics")
	FVector FallbackSunDirection = FVector(0.35f, 0.15f, -0.92f);

private:
	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> DynamicMaterial;

	// Re-entry guard: PostEditChangeProperty fires whenever a UPROPERTY mutates,
	// and RefreshDecal sets multiple component properties that can trigger
	// another PostEditChangeProperty. Without this guard the editor stack-
	// overflows when the caustic actor (or its parent ocean) is opened.
	bool bRefreshingDecal = false;

	void RefreshDecal();
	void RefreshMaterialInstance();
	void ApplyMaterialParameters();
};
