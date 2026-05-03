// Copyright (c) Myika AI. All rights reserved.

#include "MyikaLakeActor.h"

#include "Components/ChildActorComponent.h"
#include "Materials/MaterialInterface.h"
#include "MyikaCausticDecalActor.h"
#include "UObject/SoftObjectPath.h"
#include "UObject/UnrealType.h"
#include "WaterBodyComponent.h"
#include "WaterSplineComponent.h"

DEFINE_LOG_CATEGORY_STATIC(LogMyikaLake, Log, All);

namespace MyikaWater::Defaults
{
	static const TCHAR* LakeMaterialPath = TEXT("/Myika/MyikaWater/Materials/MI_MyikaWater_Lake.MI_MyikaWater_Lake");
}

AMyikaLake::AMyikaLake(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	LakeWaterMaterial = TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath(MyikaWater::Defaults::LakeMaterialPath));

	// Lakes stay still at the wave-source layer; subtle motion should come from the material.
	SetWaterWaves(nullptr);

	CausticActorComponent = CreateDefaultSubobject<UChildActorComponent>(TEXT("CausticActor"));
	CausticActorComponent->SetupAttachment(GetRootComponent());
	CausticActorComponent->SetMobility(EComponentMobility::Movable);
	CausticActorComponent->SetChildActorClass(AMyikaCausticDecalActor::StaticClass());
	CausticActorComponent->SetRelativeLocation(FVector(0.f, 0.f, CausticVerticalOffset));
	CausticActorComponent->SetRelativeRotation(FRotator(-90.f, 0.f, 0.f));
}

void AMyikaLake::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	ApplyLakeDefaults();
	RefreshCausticChild();
}

void AMyikaLake::BeginPlay()
{
	Super::BeginPlay();
	RefreshCausticChild();
}

void AMyikaLake::RefreshLakeMaterial()
{
	ApplyLakeDefaults();
}

void AMyikaLake::RefreshCausticChild()
{
	if (CausticActorComponent == nullptr)
	{
		return;
	}

	if (!bAutoSpawnCaustics)
	{
		CausticActorComponent->SetChildActorClass(nullptr);
		return;
	}

	CausticActorComponent->SetChildActorClass(AMyikaCausticDecalActor::StaticClass());
	CausticActorComponent->SetRelativeLocation(FVector(0.f, 0.f, CausticVerticalOffset));
	CausticActorComponent->SetRelativeRotation(FRotator(-90.f, 0.f, 0.f));

	if (CausticActorComponent->IsRegistered() && (CausticActorComponent->GetChildActor() == nullptr))
	{
		CausticActorComponent->CreateChildActor();
	}

	if (AMyikaCausticDecalActor* CausticActor = Cast<AMyikaCausticDecalActor>(CausticActorComponent->GetChildActor()))
	{
		CausticActor->SetProjectionExtent(ResolveCausticExtent());
	}
}

void AMyikaLake::PostLoad()
{
	Super::PostLoad();
	ApplyLakeDefaults();
	RefreshCausticChild();
}

#if WITH_EDITOR
void AMyikaLake::PostActorCreated()
{
	Super::PostActorCreated();
	ApplyLakeDefaults();
	RefreshCausticChild();
}

void AMyikaLake::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	ApplyLakeDefaults();
	RefreshCausticChild();
}
#endif

void AMyikaLake::ApplyLakeDefaults()
{
	EnsureDefaultSplineShape();

	UWaterBodyComponent* LakeWaterBodyComponent = GetWaterBodyComponent();
	if (LakeWaterBodyComponent == nullptr)
	{
		return;
	}

	UMaterialInterface* LakeMaterial = LakeWaterMaterial.Get();
	if ((LakeMaterial == nullptr) && !LakeWaterMaterial.IsNull())
	{
		LakeMaterial = LakeWaterMaterial.LoadSynchronous();
	}

	if (LakeMaterial != nullptr)
	{
		LakeWaterBodyComponent->SetWaterMaterial(LakeMaterial);
		return;
	}

	static bool bLoggedMissingLakeMaterial = false;
	if (!bLoggedMissingLakeMaterial)
	{
		bLoggedMissingLakeMaterial = true;
		UE_LOG(
			LogMyikaLake,
			Warning,
			TEXT("AMyikaLake could not load %s yet. The lake actor compiles and places cleanly, but the Myika lake material asset still needs to exist for visual proof."),
			MyikaWater::Defaults::LakeMaterialPath);
	}
}

void AMyikaLake::EnsureDefaultSplineShape()
{
	UWaterSplineComponent* WaterSpline = GetWaterSpline();
	if ((WaterSpline == nullptr) || (WaterSpline->GetNumberOfSplinePoints() > 1))
	{
		return;
	}

	WaterSpline->ClearSplinePoints(false);
	WaterSpline->AddSplinePoint(FVector(0.f, 0.f, 0.f), ESplineCoordinateSpace::Local, false);
	WaterSpline->AddSplinePoint(FVector(7000.f, -3000.f, 0.f), ESplineCoordinateSpace::Local, false);
	WaterSpline->AddSplinePoint(FVector(6500.f, 6500.f, 0.f), ESplineCoordinateSpace::Local, false);
	WaterSpline->SetClosedLoop(true, false);
	WaterSpline->UpdateSpline();
	WaterSpline->K2_SynchronizeAndBroadcastDataChange();
}

FVector AMyikaLake::ResolveCausticExtent() const
{
	if (const UWaterBodyComponent* WaterComponent = GetWaterBodyComponent())
	{
		const FBox CollisionBounds = WaterComponent->GetCollisionComponentBounds();
		if (CollisionBounds.IsValid)
		{
			const FVector Extent = CollisionBounds.GetExtent();
			return FVector(
				FMath::Max(256.f, Extent.X * CausticExtentScale),
				FMath::Max(256.f, Extent.Y * CausticExtentScale),
				FMath::Max(512.f, Extent.Z));
		}
	}

	FVector Origin = FVector::ZeroVector;
	FVector Extent = FVector(4096.f, 4096.f, 2048.f);
	GetActorBounds(false, Origin, Extent);

	return FVector(
		FMath::Max(256.f, Extent.X * CausticExtentScale),
		FMath::Max(256.f, Extent.Y * CausticExtentScale),
		FMath::Max(512.f, Extent.Z));
}
