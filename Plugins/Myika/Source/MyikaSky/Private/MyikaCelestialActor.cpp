// Copyright (c) Myika AI. All rights reserved.

#include "MyikaCelestialActor.h"

#include "Components/DirectionalLightComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"
#include "UObject/SoftObjectPath.h"

namespace
{
	const FName NAME_VisibilityAlpha(TEXT("VisibilityAlpha"));
	constexpr float EngineSphereRadius = 50.0f;
}

AMyikaCelestialActor::AMyikaCelestialActor()
{
	PrimaryActorTick.bCanEverTick = false;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	StarsSphere = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StarsSphere"));
	StarsSphere->SetupAttachment(Root);
	StarsSphere->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
	StarsSphere->SetGenerateOverlapEvents(false);
	StarsSphere->CastShadow = false;
	StarsSphere->SetMobility(EComponentMobility::Movable);

	MoonMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MoonMesh"));
	MoonMesh->SetupAttachment(Root);
	MoonMesh->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
	MoonMesh->SetGenerateOverlapEvents(false);
	MoonMesh->CastShadow = false;
	MoonMesh->SetMobility(EComponentMobility::Movable);

	MoonLight = CreateDefaultSubobject<UDirectionalLightComponent>(TEXT("MoonLight"));
	MoonLight->SetupAttachment(Root);
	MoonLight->SetMobility(EComponentMobility::Movable);
	MoonLight->SetAtmosphereSunLight(true);
	MoonLight->SetAtmosphereSunLightIndex(1);
	MoonLight->SetIntensity(0.0f);
	MoonLight->SetLightColor(MoonLightColor, true);

	static ConstructorHelpers::FObjectFinderOptional<UStaticMesh> SphereMesh(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	if (SphereMesh.Succeeded())
	{
		StarsSphere->SetStaticMesh(SphereMesh.Get());
		MoonMesh->SetStaticMesh(SphereMesh.Get());
	}

	StarsMaterialAsset = TSoftObjectPtr<UMaterialInterface>(
		FSoftObjectPath(TEXT("/Myika/MyikaSky/Materials/M_MyikaStars.M_MyikaStars")));
	MoonMaterialAsset = TSoftObjectPtr<UMaterialInterface>(
		FSoftObjectPath(TEXT("/Myika/MyikaSky/Materials/M_MyikaMoon.M_MyikaMoon")));

	ApplyScaleSettings();
}

void AMyikaCelestialActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	ApplyScaleSettings();
	RefreshAssetBindings();
}

void AMyikaCelestialActor::ApplyCelestialState(const FVector& InMoonDirection, float InVisibilityAlpha, bool bShouldRenderCelestials)
{
	RefreshAssetBindings();

	CurrentMoonDirection = InMoonDirection.IsNearlyZero()
		? FVector(0.0f, 0.0f, -1.0f)
		: InMoonDirection.GetSafeNormal();
	CurrentVisibilityAlpha = FMath::Clamp(InVisibilityAlpha, 0.0f, 1.0f);

	MoonMesh->SetRelativeLocation(-CurrentMoonDirection * MoonOrbitDistance);
	MoonLight->SetRelativeRotation(CurrentMoonDirection.Rotation());
	MoonLight->SetIntensity(MoonLightMaxIntensity * CurrentVisibilityAlpha);
	MoonLight->SetLightColor(MoonLightColor, true);

	if (StarsMaterialInstance)
	{
		StarsMaterialInstance->SetScalarParameterValue(NAME_VisibilityAlpha, CurrentVisibilityAlpha);
	}

	if (MoonMaterialInstance)
	{
		MoonMaterialInstance->SetScalarParameterValue(NAME_VisibilityAlpha, CurrentVisibilityAlpha);
	}

	const bool bHasFadeMaterials = (StarsMaterialInstance != nullptr) && (MoonMaterialInstance != nullptr);
	const bool bVisible = bShouldRenderCelestials && (bHasFadeMaterials || CurrentVisibilityAlpha > KINDA_SMALL_NUMBER);

	StarsSphere->SetVisibility(bVisible, true);
	MoonMesh->SetVisibility(bVisible, true);
	MoonLight->SetVisibility(bVisible, true);
}

void AMyikaCelestialActor::ApplyScaleSettings()
{
	const float StarsScale = FMath::Max(StarsSphereRadius / EngineSphereRadius, 1.0f);
	StarsSphere->SetRelativeScale3D(FVector(-StarsScale, StarsScale, StarsScale));

	const float MoonScale = FMath::Max(MoonRadius / EngineSphereRadius, 1.0f);
	MoonMesh->SetRelativeScale3D(FVector(MoonScale));
}

void AMyikaCelestialActor::RefreshAssetBindings()
{
	auto BindMaterial = [this](UStaticMeshComponent* TargetComponent, const TSoftObjectPtr<UMaterialInterface>& MaterialAsset, TObjectPtr<UMaterialInstanceDynamic>& OutInstance)
	{
		if (OutInstance || MaterialAsset.IsNull())
		{
			return;
		}

		if (UMaterialInterface* ResolvedMaterial = MaterialAsset.LoadSynchronous())
		{
			OutInstance = UMaterialInstanceDynamic::Create(ResolvedMaterial, this);
			TargetComponent->SetMaterial(0, OutInstance);
		}
	};

	BindMaterial(StarsSphere, StarsMaterialAsset, StarsMaterialInstance);
	BindMaterial(MoonMesh, MoonMaterialAsset, MoonMaterialInstance);
}
