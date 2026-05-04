// Copyright (c) Myika AI. All rights reserved.

#include "MyikaOceanActor.h"

#include "Components/ChildActorComponent.h"
#include "Materials/MaterialInterface.h"
#include "MyikaCausticDecalActor.h"
#include "WaterBodyComponent.h"
#include "WaterBodyOceanComponent.h"

DEFINE_LOG_CATEGORY_STATIC(LogMyikaOcean, Log, All);

namespace
{
	// 2026-05-03: now points at the Myika-authored Single Layer Water instance.
	// MI_MyikaWater_Ocean was rebuilt by tools/Create-MyikaWaterAssets.py to
	// inherit from M_MyikaWater (real Material with the full SLW graph:
	// WaterTint, dual normal sample with directional UV scrolling, scattering
	// + absorption + WaterAlbedo + WaterPhaseG SLW outputs, T_MyikaWater_Normal
	// procedural normal texture). MI override knobs:
	//   WaveSteepness=0.6, WaveSpeed=0.08, NormalTiling=0.5, ExtinctionScale=1.0,
	//   Roughness=0.04, WaterTint=(0.01,0.12,0.22), ScatteringColor=(0.30,0.45,0.45),
	//   AbsorptionColor=(0.55,0.30,0.15)
	const TCHAR* DefaultOceanMaterialPath = TEXT("/Myika/MyikaWater/Materials/MI_MyikaWater_Ocean.MI_MyikaWater_Ocean");
}

AMyikaOcean::AMyikaOcean(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	WaterMaterialAsset = TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath(DefaultOceanMaterialPath));

	CausticActorComponent = CreateDefaultSubobject<UChildActorComponent>(TEXT("CausticActor"));
	CausticActorComponent->SetupAttachment(GetRootComponent());
	CausticActorComponent->SetMobility(EComponentMobility::Movable);
	CausticActorComponent->SetChildActorClass(AMyikaCausticDecalActor::StaticClass());
	CausticActorComponent->SetRelativeLocation(FVector(0.f, 0.f, CausticVerticalOffset));
	CausticActorComponent->SetRelativeRotation(FRotator(-90.f, 0.f, 0.f));
}

void AMyikaOcean::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	ApplyDefaultWaterMaterial();
	RefreshCausticChild();
}

void AMyikaOcean::BeginPlay()
{
	Super::BeginPlay();
	RefreshCausticChild();
}

void AMyikaOcean::PostLoad()
{
	Super::PostLoad();
	ApplyDefaultWaterMaterial();
	RefreshCausticChild();
}

#if WITH_EDITOR
void AMyikaOcean::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	ApplyDefaultWaterMaterial();
	RefreshCausticChild();
}
#endif

bool AMyikaOcean::ApplyDefaultWaterMaterial()
{
	if (bApplyingDefaultMaterial)
	{
		return false;
	}
	TGuardValue<bool> ReentryGuard(bApplyingDefaultMaterial, true);

	UWaterBodyComponent* OceanWaterComponent = GetWaterBodyComponent();
	if (OceanWaterComponent == nullptr || WaterMaterialAsset.IsNull())
	{
		return false;
	}

	UMaterialInterface* Material = WaterMaterialAsset.LoadSynchronous();
	if (Material == nullptr)
	{
		UE_LOG(
			LogMyikaOcean,
			Verbose,
			TEXT("AMyikaOcean could not load default water material '%s'."),
			*WaterMaterialAsset.ToSoftObjectPath().ToString());
		return false;
	}

	if (OceanWaterComponent->GetWaterMaterial() != Material)
	{
		OceanWaterComponent->SetWaterMaterial(Material);
	}

	return true;
}

void AMyikaOcean::RefreshCausticChild()
{
	if (bRefreshingCausticChild)
	{
		return;
	}
	TGuardValue<bool> ReentryGuard(bRefreshingCausticChild, true);

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

FVector AMyikaOcean::ResolveCausticExtent() const
{
	if (const UWaterBodyOceanComponent* OceanComponent = Cast<UWaterBodyOceanComponent>(GetWaterBodyComponent()))
	{
		const FVector CollisionExtents = OceanComponent->GetCollisionExtents();
		if (!CollisionExtents.IsNearlyZero())
		{
			return FVector(
				FMath::Max(512.f, CollisionExtents.X * CausticExtentScale),
				FMath::Max(512.f, CollisionExtents.Y * CausticExtentScale),
				FMath::Max(1024.f, CollisionExtents.Z));
		}
	}

	FVector Origin = FVector::ZeroVector;
	FVector Extent = FVector(8192.f, 8192.f, 4096.f);
	GetActorBounds(false, Origin, Extent);

	return FVector(
		FMath::Max(512.f, Extent.X * CausticExtentScale),
		FMath::Max(512.f, Extent.Y * CausticExtentScale),
		FMath::Max(1024.f, Extent.Z));
}
