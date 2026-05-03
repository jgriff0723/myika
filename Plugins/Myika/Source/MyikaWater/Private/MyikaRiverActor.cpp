// Copyright (c) Myika AI. All rights reserved.

#include "MyikaRiverActor.h"

#include "Components/DecalComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "WaterBodyComponent.h"
#include "WaterBodyRiverComponent.h"
#include "WaterSplineComponent.h"

DEFINE_LOG_CATEGORY_STATIC(LogMyikaRiver, Log, All);

namespace
{
	const FName FlowDirectionParameterName(TEXT("FlowDirection"));
	const FName FlowStrengthParameterName(TEXT("FlowStrength"));
	const FName FoamRadiusParameterName(TEXT("FoamRadius"));
}

AMyikaRiver::AMyikaRiver(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryActorTick.bCanEverTick = false;
	ApplyRiverDefaults();
}

void AMyikaRiver::BeginPlay()
{
	Super::BeginPlay();
	RefreshRiver();
}

void AMyikaRiver::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	RefreshRiver();
}

#if WITH_EDITOR
void AMyikaRiver::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	RefreshRiver();
}
#endif

void AMyikaRiver::RefreshRiver()
{
	ApplyRiverDefaults();
	SynchronizeFlowFromSpline();
	RebuildObstacleFoamDecals();
}

void AMyikaRiver::RefreshObstacleFoam()
{
	RebuildObstacleFoamDecals();
}

void AMyikaRiver::ApplyRiverDefaults()
{
	if (UWaterBodyRiverComponent* RiverComponent = GetRiverComponent())
	{
		RiverComponent->bAffectsLandscape = bEnableLandscapeCarving;

		if (RiverWaterMaterial != nullptr)
		{
			RiverComponent->SetWaterMaterial(RiverWaterMaterial);
		}
	}
}

void AMyikaRiver::SynchronizeFlowFromSpline()
{
	if (!bAutoSyncFlowFromSpline)
	{
		return;
	}

	UWaterBodyRiverComponent* RiverComponent = GetRiverComponent();
	UWaterSplineComponent* WaterSpline = GetWaterSpline();
	if ((RiverComponent == nullptr) || (WaterSpline == nullptr))
	{
		return;
	}

	const int32 NumPoints = WaterSpline->GetNumberOfSplinePoints();
	if (NumPoints < 2)
	{
		return;
	}

	for (int32 PointIndex = 0; PointIndex < NumPoints; ++PointIndex)
	{
		const float Velocity = ComputeVelocityForSplinePoint(WaterSpline, PointIndex);
		RiverComponent->SetWaterVelocityAtSplineInputKey(static_cast<float>(PointIndex), Velocity);
	}

	WaterSpline->K2_SynchronizeAndBroadcastDataChange();
}

void AMyikaRiver::RebuildObstacleFoamDecals()
{
	DestroyObstacleFoamDecals();

	if (!bSpawnObstacleFoamDecals || (FoamDecalMaterial == nullptr))
	{
		return;
	}

	UWaterSplineComponent* WaterSpline = GetWaterSpline();
	UWaterBodyComponent* RiverWaterBodyComponent = GetWaterBodyComponent();
	if ((WaterSpline == nullptr) || (RiverWaterBodyComponent == nullptr))
	{
		return;
	}

	TArray<AActor*> Obstacles;
	UGameplayStatics::GetAllActorsWithTag(this, RiverObstacleTag, Obstacles);

	USceneComponent* AttachmentRoot = GetRootComponent();
	if (AttachmentRoot == nullptr)
	{
		AttachmentRoot = WaterSpline;
	}

	for (AActor* ObstacleActor : Obstacles)
	{
		if ((ObstacleActor == nullptr) || (ObstacleActor == this))
		{
			continue;
		}

		FVector SurfaceLocation = FVector::ZeroVector;
		FVector SurfaceNormal = FVector::UpVector;
		FVector WaterVelocity = FVector::ZeroVector;
		float WaterDepth = 0.0f;
		if (!TryGetRiverSurfaceInfo(ObstacleActor->GetActorLocation(), SurfaceLocation, SurfaceNormal, WaterVelocity, WaterDepth))
		{
			continue;
		}

		if (FVector::DistXY(ObstacleActor->GetActorLocation(), SurfaceLocation) > ObstacleMaxDistanceFromRiver)
		{
			continue;
		}

		const float ClosestInputKey = RiverWaterBodyComponent->FindInputKeyClosestToWorldLocation(ObstacleActor->GetActorLocation());
		const int32 ClosestPointIndex = FMath::Clamp(FMath::RoundToInt(ClosestInputKey), 0, WaterSpline->GetNumberOfSplinePoints() - 1);
		const FVector FlowDirection = WaterVelocity.IsNearlyZero()
			? GetFallbackFlowDirection(WaterSpline, ClosestPointIndex)
			: WaterVelocity.GetSafeNormal();

		UDecalComponent* FoamDecal = NewObject<UDecalComponent>(this);
		if (FoamDecal == nullptr)
		{
			continue;
		}

		FoamDecal->SetMobility(EComponentMobility::Movable);
		FoamDecal->SetWorldLocation(SurfaceLocation + (SurfaceNormal * FoamVerticalOffset));
		FoamDecal->SetWorldRotation((-SurfaceNormal).Rotation());
		FoamDecal->DecalSize = FoamDecalSize;
		FoamDecal->SetFadeScreenSize(0.001f);
		FoamDecal->SetSortOrder(1);

		UMaterialInstanceDynamic* FoamMid = UMaterialInstanceDynamic::Create(FoamDecalMaterial, FoamDecal);
		if (FoamMid != nullptr)
		{
			FoamMid->SetVectorParameterValue(FlowDirectionParameterName, FLinearColor(FlowDirection.X, FlowDirection.Y, FlowDirection.Z, 1.0f));
			FoamMid->SetScalarParameterValue(FlowStrengthParameterName, WaterVelocity.Size() * FoamFlowStrengthScale);
			FoamMid->SetScalarParameterValue(FoamRadiusParameterName, FoamDecalSize.Y);
			FoamDecal->SetDecalMaterial(FoamMid);
		}
		else
		{
			FoamDecal->SetDecalMaterial(FoamDecalMaterial);
		}

		FoamDecal->SetupAttachment(AttachmentRoot);
		FoamDecal->RegisterComponent();
		SpawnedFoamDecals.Add(FoamDecal);
	}

	UE_LOG(LogMyikaRiver, Verbose, TEXT("AMyikaRiver rebuilt %d obstacle foam decals."), SpawnedFoamDecals.Num());
}

void AMyikaRiver::DestroyObstacleFoamDecals()
{
	for (UDecalComponent* FoamDecal : SpawnedFoamDecals)
	{
		if (FoamDecal != nullptr)
		{
			FoamDecal->DestroyComponent();
		}
	}

	SpawnedFoamDecals.Reset();
}

UWaterBodyRiverComponent* AMyikaRiver::GetRiverComponent() const
{
	return Cast<UWaterBodyRiverComponent>(GetWaterBodyComponent());
}

float AMyikaRiver::ComputeVelocityForSplinePoint(const UWaterSplineComponent* WaterSpline, int32 PointIndex) const
{
	const int32 NumPoints = WaterSpline->GetNumberOfSplinePoints();
	if (NumPoints <= 1)
	{
		return MinFlowVelocity;
	}

	const int32 PreviousPointIndex = FMath::Max(PointIndex - 1, 0);
	const int32 NextPointIndex = FMath::Min(PointIndex + 1, NumPoints - 1);

	const FVector CurrentLocation = WaterSpline->GetLocationAtSplinePoint(PointIndex, ESplineCoordinateSpace::World);
	const FVector PreviousLocation = WaterSpline->GetLocationAtSplinePoint(PreviousPointIndex, ESplineCoordinateSpace::World);
	const FVector NextLocation = WaterSpline->GetLocationAtSplinePoint(NextPointIndex, ESplineCoordinateSpace::World);

	const float PreviousSegmentLength = (PointIndex > 0) ? FVector::Dist(CurrentLocation, PreviousLocation) : 0.0f;
	const float NextSegmentLength = (PointIndex < (NumPoints - 1)) ? FVector::Dist(CurrentLocation, NextLocation) : PreviousSegmentLength;
	const float SegmentLength = FMath::Max((PreviousSegmentLength + NextSegmentLength) * 0.5f, 1.0f);

	const FVector SplineTangent = WaterSpline->GetTangentAtSplinePoint(PointIndex, ESplineCoordinateSpace::World).GetSafeNormal();
	const float SlopeFactor = FMath::Clamp(FMath::Abs(SplineTangent.Z), 0.0f, 1.0f);

	const float BaseVelocity = (SegmentLength / 100.0f) * FlowSpeedScale;
	const float BoostedVelocity = BaseVelocity * (1.0f + (SlopeFactor * SlopeVelocityBoost));
	return FMath::Clamp(BoostedVelocity, MinFlowVelocity, MaxFlowVelocity);
}

FVector AMyikaRiver::GetFallbackFlowDirection(const UWaterSplineComponent* WaterSpline, int32 PointIndex) const
{
	return WaterSpline->GetTangentAtSplinePoint(PointIndex, ESplineCoordinateSpace::World).GetSafeNormal();
}

bool AMyikaRiver::TryGetRiverSurfaceInfo(const FVector& WorldLocation, FVector& OutSurfaceLocation, FVector& OutSurfaceNormal,
	FVector& OutVelocity, float& OutDepth) const
{
	if (const UWaterBodyComponent* RiverWaterBodyComponent = GetWaterBodyComponent())
	{
		return RiverWaterBodyComponent->GetWaterSurfaceInfoAtLocation(WorldLocation, OutSurfaceLocation, OutSurfaceNormal, OutVelocity, OutDepth, true);
	}

	return false;
}
