// Copyright (c) Myika AI. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "WaterBodyRiverActor.h"
#include "MyikaRiverActor.generated.h"

class UDecalComponent;
class UMaterialInterface;
class UWaterBodyRiverComponent;
class UWaterSplineComponent;

UCLASS(Blueprintable, BlueprintType, ClassGroup = (Myika), meta = (DisplayName = "Myika River"))
class MYIKAWATER_API AMyikaRiver : public AWaterBodyRiver
{
	GENERATED_BODY()

public:
	AMyikaRiver(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual void BeginPlay() override;
	virtual void OnConstruction(const FTransform& Transform) override;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

	/** Re-apply river defaults, spline-flow metadata, and obstacle foam decals. */
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Myika|Water|River")
	void RefreshRiver();

	/** Rebuild only the obstacle foam decals after obstacle placement or tag edits. */
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Myika|Water|River")
	void RefreshObstacleFoam();

protected:
	/** Optional material override pushed onto the wrapped UE Water river component. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Myika|Water|River|Surface")
	TObjectPtr<UMaterialInterface> RiverWaterMaterial = nullptr;

	/** Keep UE Water spline velocity metadata synchronized from spline shape and spacing. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Myika|Water|River|Flow")
	bool bAutoSyncFlowFromSpline = true;

	/** Multiplier applied when converting spline spacing into river flow speed. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Myika|Water|River|Flow",
		meta = (ClampMin = "0.0", UIMin = "0.0", UIMax = "8.0", SliderExponent = "2.0",
			EditCondition = "bAutoSyncFlowFromSpline"))
	float FlowSpeedScale = 1.0f;

	/** Minimum water velocity written into spline metadata when flow sync runs. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Myika|Water|River|Flow",
		meta = (ClampMin = "0.0", UIMin = "0.0", UIMax = "1024.0",
			EditCondition = "bAutoSyncFlowFromSpline"))
	float MinFlowVelocity = 96.0f;

	/** Maximum water velocity written into spline metadata when flow sync runs. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Myika|Water|River|Flow",
		meta = (ClampMin = "0.0", UIMin = "0.0", UIMax = "4096.0",
			EditCondition = "bAutoSyncFlowFromSpline"))
	float MaxFlowVelocity = 640.0f;

	/** Additional boost applied when spline tangents pitch sharply up or down. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Myika|Water|River|Flow",
		meta = (ClampMin = "0.0", UIMin = "0.0", UIMax = "4.0", SliderExponent = "2.0",
			EditCondition = "bAutoSyncFlowFromSpline"))
	float SlopeVelocityBoost = 1.0f;

	/** Preserve landscape carving and bank shaping on the wrapped UE Water river body. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Myika|Water|River|Landscape")
	bool bEnableLandscapeCarving = true;

	/** Automatically stamp foam decals around actors tagged as river obstacles. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Myika|Water|River|Foam")
	bool bSpawnObstacleFoamDecals = true;

	/** Tag searched when collecting obstacle actors that should receive local foam decals. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Myika|Water|River|Foam",
		meta = (EditCondition = "bSpawnObstacleFoamDecals"))
	FName RiverObstacleTag = TEXT("RiverObstacle");

	/** Material used by the optional obstacle foam decals. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Myika|Water|River|Foam",
		meta = (EditCondition = "bSpawnObstacleFoamDecals"))
	TObjectPtr<UMaterialInterface> FoamDecalMaterial = nullptr;

	/** Maximum XY distance from the river centerline where an obstacle still receives foam. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Myika|Water|River|Foam",
		meta = (ClampMin = "0.0", UIMin = "0.0", UIMax = "2048.0",
			EditCondition = "bSpawnObstacleFoamDecals"))
	float ObstacleMaxDistanceFromRiver = 384.0f;

	/** Size of spawned foam decals around detected obstacles. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Myika|Water|River|Foam",
		meta = (EditCondition = "bSpawnObstacleFoamDecals"))
	FVector FoamDecalSize = FVector(96.0f, 192.0f, 192.0f);

	/** Offset applied above the sampled water surface when placing foam decals. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Myika|Water|River|Foam",
		meta = (ClampMin = "-100.0", ClampMax = "100.0", UIMin = "-32.0", UIMax = "32.0",
			EditCondition = "bSpawnObstacleFoamDecals"))
	float FoamVerticalOffset = 8.0f;

	/** Multiplier pushed into the decal material when exposing local flow intensity. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Myika|Water|River|Foam",
		meta = (ClampMin = "0.0", UIMin = "0.0", UIMax = "4.0", SliderExponent = "2.0",
			EditCondition = "bSpawnObstacleFoamDecals"))
	float FoamFlowStrengthScale = 0.01f;

	/** Transient decal components owned by this actor for obstacle-local foam. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "Myika|Debug",
		meta = (AllowPrivateAccess = "true"))
	TArray<TObjectPtr<UDecalComponent>> SpawnedFoamDecals;

private:
	void ApplyRiverDefaults();
	void SynchronizeFlowFromSpline();
	void RebuildObstacleFoamDecals();
	void DestroyObstacleFoamDecals();

	UWaterBodyRiverComponent* GetRiverComponent() const;
	float ComputeVelocityForSplinePoint(const UWaterSplineComponent* WaterSpline, int32 PointIndex) const;
	FVector GetFallbackFlowDirection(const UWaterSplineComponent* WaterSpline, int32 PointIndex) const;
	bool TryGetRiverSurfaceInfo(const FVector& WorldLocation, FVector& OutSurfaceLocation, FVector& OutSurfaceNormal,
		FVector& OutVelocity, float& OutDepth) const;
};
