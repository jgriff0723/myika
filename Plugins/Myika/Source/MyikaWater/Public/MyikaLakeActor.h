// Copyright (c) Myika AI. All rights reserved.
//
// Calm-water wrapper over UE Water's AWaterBodyLake. Applies the Myika lake material when
// the asset exists and keeps a default spline footprint so artists can place it immediately.

#pragma once

#include "CoreMinimal.h"
#include "UObject/SoftObjectPtr.h"
#include "WaterBodyLakeActor.h"
#include "MyikaLakeActor.generated.h"

class UChildActorComponent;
class UMaterialInterface;
struct FPropertyChangedEvent;

UCLASS(BlueprintType, Blueprintable, ClassGroup = (Myika), meta = (DisplayName = "Myika Lake"))
class MYIKAWATER_API AMyikaLake : public AWaterBodyLake
{
	GENERATED_BODY()

public:
	AMyikaLake(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;

	/** Reapplies the configured lake surface material to the wrapped water body component. */
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Myika|Water|Surface")
	void RefreshLakeMaterial();

	/** Rebuild the child caustic actor after spline or extent changes. */
	UFUNCTION(BlueprintCallable, Category = "Myika|Water|Caustics")
	void RefreshCausticChild();

protected:
	virtual void PostLoad() override;

#if WITH_EDITOR
	virtual void PostActorCreated() override;
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

	/** Default still-water surface material. Assigns to the lake water body when the asset exists. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Myika|Water|Surface", meta = (AllowedClasses = "/Script/Engine.MaterialInterface"))
	TSoftObjectPtr<UMaterialInterface> LakeWaterMaterial;

	/** Spawn a projected caustic child actor under the lake surface. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Myika|Water|Caustics")
	TObjectPtr<UChildActorComponent> CausticActorComponent;

	/** Whether this lake should spawn its companion caustic decal automatically. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Myika|Water|Caustics")
	bool bAutoSpawnCaustics = true;

	/** Scales the spline/collision-derived projection box so calm lakes can tighten or broaden the decal reach. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Myika|Water|Caustics",
		meta = (ClampMin = "0.25", ClampMax = "4.0", UIMin = "0.5", UIMax = "2.0", SliderExponent = "2.0"))
	float CausticExtentScale = 1.0f;

	/** Offsets the caustic projector downward from the water surface. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Myika|Water|Caustics",
		meta = (ClampMin = "-50000.0", ClampMax = "50000.0", UIMin = "-5000.0", UIMax = "5000.0"))
	float CausticVerticalOffset = -150.0f;

private:
	void ApplyLakeDefaults();
	void EnsureDefaultSplineShape();
	FVector ResolveCausticExtent() const;

	// Re-entry guards — match MyikaOceanActor's pattern. SetWaterMaterial /
	// CreateChildActor / SetMobility can each fire PostEditChangeProperty or
	// re-enter OnConstruction in the editor; without the guards opening the
	// lake actor in the editor blows the stack.
	bool bApplyingLakeDefaults = false;
	bool bRefreshingCausticChild = false;
};
