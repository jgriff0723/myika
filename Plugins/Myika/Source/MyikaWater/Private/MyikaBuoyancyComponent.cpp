// Copyright (c) Myika AI. All rights reserved.

#include "MyikaBuoyancyComponent.h"

#include "BuoyancyManager.h"
#include "Components/PrimitiveComponent.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#if WITH_EDITOR
#include "UObject/UnrealType.h"
#endif
#include "WaterSubsystem.h"

DEFINE_LOG_CATEGORY_STATIC(LogMyikaBuoyancy, Log, All);

UMyikaBuoyancyComponent::UMyikaBuoyancyComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UMyikaBuoyancyComponent::BeginPlay()
{
	ApplyAuthoringDefaults();
	if (bAutoConfigurePontoons)
	{
		AutoConfigurePontoons();
	}

	UActorComponent::BeginPlay();

	SimulatingComponent = ResolveTargetPrimitive();
	if (SimulatingComponent)
	{
		for (FSphericalPontoon& Pontoon : BuoyancyData.Pontoons)
		{
			if (Pontoon.CenterSocket != NAME_None && SimulatingComponent->DoesSocketExist(Pontoon.CenterSocket))
			{
				Pontoon.bUseCenterSocket = true;
				Pontoon.SocketTransform = SimulatingComponent->GetSocketTransform(Pontoon.CenterSocket, RTS_Actor);
			}
		}

		SetupWaterBodyOverlaps();
	}
	else
	{
		UE_LOG(LogMyikaBuoyancy, Warning, TEXT("%s could not resolve a primitive component for buoyancy."), *GetPathName());
	}

	FinalizeAuxData();

	if (UWorld* World = GetWorld())
	{
		if (UWaterSubsystem* WaterSubsystem = UWaterSubsystem::GetWaterSubsystem(World))
		{
			if (ABuoyancyManager* Manager = WaterSubsystem->GetBuoyancyManager())
			{
				Manager->Register(this);
			}
		}
	}
}

void UMyikaBuoyancyComponent::OnRegister()
{
	Super::OnRegister();

	if (HasAnyFlags(RF_ClassDefaultObject))
	{
		return;
	}

	ApplyAuthoringDefaults();
	if (bAutoConfigurePontoons)
	{
		AutoConfigurePontoons();
	}
	else
	{
		UpdatePontoonCoefficients();
	}
}

#if WITH_EDITOR
void UMyikaBuoyancyComponent::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	ApplyAuthoringDefaults();
	if (bAutoConfigurePontoons)
	{
		AutoConfigurePontoons();
	}
	else
	{
		UpdatePontoonCoefficients();
	}
}
#endif

void UMyikaBuoyancyComponent::RebuildPontoons()
{
	ApplyAuthoringDefaults();
	if (bAutoConfigurePontoons)
	{
		AutoConfigurePontoons();
	}
	else
	{
		UpdatePontoonCoefficients();
	}
}

UPrimitiveComponent* UMyikaBuoyancyComponent::ResolveTargetPrimitive() const
{
	const AActor* Owner = GetOwner();
	if (!Owner)
	{
		return nullptr;
	}

	TInlineComponentArray<UPrimitiveComponent*> PrimitiveComponents;
	Owner->GetComponents(PrimitiveComponents);

	UPrimitiveComponent* RootPrimitive = Cast<UPrimitiveComponent>(Owner->GetRootComponent());
	UPrimitiveComponent* FirstPrimitive = RootPrimitive;

	for (UPrimitiveComponent* Primitive : PrimitiveComponents)
	{
		if (!Primitive)
		{
			continue;
		}

		if (!FirstPrimitive)
		{
			FirstPrimitive = Primitive;
		}

		if (Primitive->IsSimulatingPhysics())
		{
			return Primitive;
		}
	}

	return RootPrimitive ? RootPrimitive : FirstPrimitive;
}

void UMyikaBuoyancyComponent::ApplyAuthoringDefaults()
{
	BuoyancyData.BuoyancyCoefficient = DensityScale;
	BuoyancyData.MaxBuoyantForce = MaxBuoyantForce;
	BuoyancyData.bApplyDragForcesInWater = true;
	BuoyancyData.DragCoefficient = LinearDrag;
	BuoyancyData.DragCoefficient2 = QuadraticDrag;
	BuoyancyData.AngularDragCoefficient = AngularDrag;
	BuoyancyData.MaxDragSpeed = 80.0f;
	BuoyancyData.bApplyRiverForces = bApplyRiverForces;
	BuoyancyData.WaterVelocityStrength = RiverVelocityStrength;
	BuoyancyData.MaxWaterForce = MaxRiverForce;
	BuoyancyData.bAlwaysAllowLateralPush = true;
	BuoyancyData.bAllowCurrentWhenMovingFastUpstream = true;
	BuoyancyData.bApplyDownstreamAngularRotation = bAlignToRiverFlow;
	BuoyancyData.DownstreamAxisOfRotation = FVector::ForwardVector;
	BuoyancyData.DownstreamRotationStrength = RiverAlignmentStrength;
	BuoyancyData.DownstreamRotationStiffness = 20.0f;
	BuoyancyData.DownstreamRotationAngularDamping = 5.0f;
	BuoyancyData.DownstreamMaxAcceleration = 12.0f;
	BuoyancyData.WaterShorePushFactor = 0.0f;
	BuoyancyData.RiverTraversalPathWidth = 600.0f;
	BuoyancyData.MaxShorePushForce = 1500.0f;
}

void UMyikaBuoyancyComponent::AutoConfigurePontoons()
{
	UPrimitiveComponent* TargetPrimitive = ResolveTargetPrimitive();
	if (!TargetPrimitive)
	{
		return;
	}

	const FBoxSphereBounds LocalBounds = TargetPrimitive->CalcBounds(FTransform::Identity);
	const FVector Extent = LocalBounds.BoxExtent.GetAbs();
	if (Extent.IsNearlyZero())
	{
		return;
	}

	const float Radius = FMath::Max(10.0f, FMath::Min(Extent.X, Extent.Y) * AutoPontoonRadiusScale);
	const float XOffset = FMath::Max(Radius, Extent.X * (1.0f - AutoPontoonInsetFraction));
	const float YOffset = FMath::Max(Radius, Extent.Y * (1.0f - AutoPontoonInsetFraction));
	const float ZOffset = (-Extent.Z + Radius) + AutoPontoonDepthBias;

	BuoyancyData.Pontoons.Reset();

	auto AddPontoon = [this, Radius](const FVector& RelativeLocation)
	{
		FSphericalPontoon Pontoon;
		Pontoon.Radius = Radius;
		Pontoon.RelativeLocation = RelativeLocation;
		Pontoon.bFXEnabled = true;
		BuoyancyData.Pontoons.Add(Pontoon);
	};

	AddPontoon(FVector( XOffset,  YOffset, ZOffset));
	AddPontoon(FVector( XOffset, -YOffset, ZOffset));
	AddPontoon(FVector(-XOffset,  YOffset, ZOffset));
	AddPontoon(FVector(-XOffset, -YOffset, ZOffset));

	if (bAddCenterPontoon)
	{
		AddPontoon(FVector(0.0f, 0.0f, ZOffset));
	}

	BuoyancyData.RiverPontoonIndex = bAddCenterPontoon ? BuoyancyData.Pontoons.Num() - 1 : 0;
	UpdatePontoonCoefficients();
}
