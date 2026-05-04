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
	// 2026-05-03 ROLLBACK: reverted to engine Water_Material_Inst.
	// The Myika-authored M_MyikaWater + MI_MyikaWater_Ocean rebuild from
	// commit 05fa6be was triggering an editor memory leak when L_SkyTest
	// opened (16GB -> 26GB+ in 20 sec, eventually OOM crash). Suspected
	// cause: cross-plugin asset registry chain when MI_MyikaWater_Ocean
	// inherits from M_MyikaWater which uses T_MyikaWater_Normal under
	// /Myika/MyikaWater/Materials/. Rolled back to the engine-material
	// state Jacob last confirmed worked (commit 8cad974). Will retry with
	// a more isolated material graph after the leak is root-caused.
	const TCHAR* DefaultOceanMaterialPath = TEXT("/Water/Materials/WaterSurface/Water_Material_Inst.Water_Material_Inst");
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
