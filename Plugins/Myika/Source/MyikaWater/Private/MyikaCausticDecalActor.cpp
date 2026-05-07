// Copyright (c) Myika AI. All rights reserved.

#include "MyikaCausticDecalActor.h"

#include "Components/DecalComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"
#include "UObject/UnrealType.h"

namespace
{
	const TCHAR* DefaultCausticMaterialPath = TEXT("/Myika/MyikaWater/Decals/M_MyikaCaustic.M_MyikaCaustic");
}

AMyikaCausticDecalActor::AMyikaCausticDecalActor()
{
	PrimaryActorTick.bCanEverTick = false;

	if (UDecalComponent* DecalComponent = GetDecal())
	{
		DecalComponent->SetMobility(EComponentMobility::Movable);
		DecalComponent->DecalSize = ProjectionExtent;
	}
}

void AMyikaCausticDecalActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	RefreshDecal();
}

void AMyikaCausticDecalActor::BeginPlay()
{
	Super::BeginPlay();
	RefreshDecal();
}

#if WITH_EDITOR
void AMyikaCausticDecalActor::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	RefreshDecal();
}
#endif

void AMyikaCausticDecalActor::SetProjectionExtent(const FVector& NewExtent)
{
	ProjectionExtent = FVector(
		FMath::Max(64.f, NewExtent.X),
		FMath::Max(64.f, NewExtent.Y),
		FMath::Max(64.f, NewExtent.Z));

	RefreshDecal();
}

void AMyikaCausticDecalActor::SetFallbackSunDirection(const FVector& NewSunDirection)
{
	FallbackSunDirection = NewSunDirection.IsNearlyZero()
		? FVector(0.f, 0.f, -1.f)
		: NewSunDirection.GetSafeNormal();
	ApplyMaterialParameters();
}

void AMyikaCausticDecalActor::RefreshDecal()
{
	if (bRefreshingDecal)
	{
		return;
	}
	TGuardValue<bool> ReentryGuard(bRefreshingDecal, true);

	if (UDecalComponent* DecalComponent = GetDecal())
	{
		DecalComponent->DecalSize = ProjectionExtent;
	}

	RefreshMaterialInstance();
	ApplyMaterialParameters();
}

void AMyikaCausticDecalActor::RefreshMaterialInstance()
{
	UDecalComponent* DecalComponent = GetDecal();
	if (DecalComponent == nullptr)
	{
		DynamicMaterial = nullptr;
		return;
	}

	if (CausticMaterial == nullptr)
	{
		CausticMaterial = LoadObject<UMaterialInterface>(nullptr, DefaultCausticMaterialPath);
	}

	if ((CausticMaterial != nullptr) && (DecalComponent->GetDecalMaterial() != DynamicMaterial))
	{
		DecalComponent->SetDecalMaterial(CausticMaterial);
		DynamicMaterial = DecalComponent->CreateDynamicMaterialInstance();
		return;
	}

	if ((DynamicMaterial == nullptr) && (DecalComponent->GetDecalMaterial() != nullptr))
	{
		DynamicMaterial = DecalComponent->CreateDynamicMaterialInstance();
	}
}

void AMyikaCausticDecalActor::ApplyMaterialParameters()
{
	if (DynamicMaterial == nullptr)
	{
		return;
	}

	DynamicMaterial->SetScalarParameterValue(TEXT("AnimationSpeed"), AnimationSpeed);
	DynamicMaterial->SetScalarParameterValue(TEXT("CausticIntensity"), CausticIntensity);
	DynamicMaterial->SetScalarParameterValue(TEXT("SunDriftStrength"), SunDriftStrength);
	DynamicMaterial->SetVectorParameterValue(TEXT("FallbackSunDirection"), FLinearColor(FallbackSunDirection));
}
