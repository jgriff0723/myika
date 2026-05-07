// Copyright (c) Myika AI. All rights reserved.

#include "MyikaUnderwaterPostProcessComponent.h"

#include "Components/PostProcessComponent.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Math/NumericLimits.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Templates/ValueOrError.h"
#include "WaterBodyActor.h"
#include "WaterBodyComponent.h"
#include "WaterBodyTypes.h"

DEFINE_LOG_CATEGORY_STATIC(LogMyikaUnderwaterPostProcess, Log, All);

UMyikaUnderwaterPostProcessComponent::UMyikaUnderwaterPostProcessComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
}

void UMyikaUnderwaterPostProcessComponent::BeginPlay()
{
	Super::BeginPlay();

	EnsurePostProcessComponent();
	RefreshMaterialInstance();
	ApplyEffectState(false, 0.0f, 0.0f);
}

void UMyikaUnderwaterPostProcessComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	PostProcessMaterialInstance = nullptr;
	PostProcessComponent = nullptr;

	Super::EndPlay(EndPlayReason);
}

void UMyikaUnderwaterPostProcessComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	EnsurePostProcessComponent();
	RefreshMaterialInstance();

	FVector ViewLocation = FVector::ZeroVector;
	if (!ResolvePlayerViewLocation(ViewLocation))
	{
		ApplyEffectState(false, 0.0f, 0.0f);
		return;
	}

	float WaterSurfaceZ = 0.0f;
	float DepthBelowSurfaceCm = 0.0f;
	const bool bHasWaterSurface = ResolveNearestWaterSurface(ViewLocation, WaterSurfaceZ, DepthBelowSurfaceCm);
	const bool bShouldEnable = bHasWaterSurface && (DepthBelowSurfaceCm > ActivationDepthBiasCm);

	ApplyEffectState(bShouldEnable, WaterSurfaceZ, DepthBelowSurfaceCm);
}

void UMyikaUnderwaterPostProcessComponent::EnsurePostProcessComponent()
{
	if (IsValid(PostProcessComponent))
	{
		PostProcessComponent->bEnabled = true;
		PostProcessComponent->bUnbound = true;
		PostProcessComponent->Priority = PostProcessPriority;
		return;
	}

	AActor* Owner = GetOwner();
	if (!IsValid(Owner))
	{
		return;
	}

	PostProcessComponent = NewObject<UPostProcessComponent>(Owner, TEXT("MyikaUnderwaterPostProcessVolume"));
	if (!IsValid(PostProcessComponent))
	{
		return;
	}

	PostProcessComponent->bEnabled = true;
	PostProcessComponent->bUnbound = true;
	PostProcessComponent->BlendRadius = 0.0f;
	PostProcessComponent->BlendWeight = 0.0f;
	PostProcessComponent->Priority = PostProcessPriority;

	if (USceneComponent* RootComponent = Owner->GetRootComponent())
	{
		PostProcessComponent->SetupAttachment(RootComponent);
	}

	Owner->AddInstanceComponent(PostProcessComponent);
	PostProcessComponent->RegisterComponent();
}

void UMyikaUnderwaterPostProcessComponent::RefreshMaterialInstance()
{
	if (!IsValid(PostProcessComponent) || !IsValid(UnderwaterPostProcessMaterial) || IsValid(PostProcessMaterialInstance))
	{
		PushMaterialParameters();
		return;
	}

	PostProcessMaterialInstance = UMaterialInstanceDynamic::Create(UnderwaterPostProcessMaterial, this, TEXT("MID_MyikaUnderwaterPP"));
	if (!IsValid(PostProcessMaterialInstance))
	{
		UE_LOG(LogMyikaUnderwaterPostProcess, Warning, TEXT("Failed to create underwater post-process MID on %s."), *GetNameSafe(GetOwner()));
		return;
	}

	TScriptInterface<IBlendableInterface> Blendable;
	Blendable.SetObject(PostProcessMaterialInstance);
	Blendable.SetInterface(static_cast<IBlendableInterface*>(PostProcessMaterialInstance));
	PostProcessComponent->AddOrUpdateBlendable(Blendable, 1.0f);

	PushMaterialParameters();
}

void UMyikaUnderwaterPostProcessComponent::PushMaterialParameters()
{
	if (!IsValid(PostProcessMaterialInstance))
	{
		return;
	}

	PostProcessMaterialInstance->SetVectorParameterValue(TEXT("WaterColor"), WaterColor);
	PostProcessMaterialInstance->SetScalarParameterValue(TEXT("FogDensity"), FogDensity);
	PostProcessMaterialInstance->SetScalarParameterValue(TEXT("RefractionStrength"), RefractionStrength);
	PostProcessMaterialInstance->SetScalarParameterValue(TEXT("WaterlineSmoothness"), WaterlineSmoothness);
	PostProcessMaterialInstance->SetScalarParameterValue(TEXT("DepthBelowSurfaceCm"), CurrentDepthBelowSurfaceCm);
	PostProcessMaterialInstance->SetScalarParameterValue(TEXT("WaterSurfaceZ"), CurrentWaterSurfaceZ);
	PostProcessMaterialInstance->SetScalarParameterValue(TEXT("UnderwaterWeight"), bEffectActive ? 1.0f : 0.0f);
}

bool UMyikaUnderwaterPostProcessComponent::ResolvePlayerViewLocation(FVector& OutViewLocation) const
{
	const APawn* OwnerPawn = Cast<APawn>(GetOwner());
	if (IsValid(OwnerPawn))
	{
		if (const APlayerController* OwnerController = Cast<APlayerController>(OwnerPawn->GetController()))
		{
			FRotator ViewRotation = FRotator::ZeroRotator;
			OwnerController->GetPlayerViewPoint(OutViewLocation, ViewRotation);
			return true;
		}
	}

	if (const UWorld* World = GetWorld())
	{
		if (const APlayerController* FirstPlayerController = World->GetFirstPlayerController())
		{
			FRotator ViewRotation = FRotator::ZeroRotator;
			FirstPlayerController->GetPlayerViewPoint(OutViewLocation, ViewRotation);
			return true;
		}
	}

	return false;
}

bool UMyikaUnderwaterPostProcessComponent::ResolveNearestWaterSurface(const FVector& ViewLocation, float& OutSurfaceZ, float& OutDepthBelowSurfaceCm) const
{
	const UWorld* World = GetWorld();
	if (!IsValid(World))
	{
		return false;
	}

	EWaterBodyQueryFlags QueryFlags = EWaterBodyQueryFlags::ComputeLocation | EWaterBodyQueryFlags::ComputeImmersionDepth;
	if (bIncludeWaves)
	{
		QueryFlags |= EWaterBodyQueryFlags::IncludeWaves;
	}

	bool bFoundWaterBody = false;
	double BestDistanceSq = TNumericLimits<double>::Max();

	for (TActorIterator<AWaterBody> WaterBodyIt(World); WaterBodyIt; ++WaterBodyIt)
	{
		const AWaterBody* WaterBody = *WaterBodyIt;
		if (!IsValid(WaterBody))
		{
			continue;
		}

		const UWaterBodyComponent* WaterBodyComponent = WaterBody->GetWaterBodyComponent();
		if (!IsValid(WaterBodyComponent))
		{
			continue;
		}

		const TValueOrError<FWaterBodyQueryResult, EWaterBodyQueryError> QueryResult =
			WaterBodyComponent->TryQueryWaterInfoClosestToWorldLocation(ViewLocation, QueryFlags);
		if (!QueryResult.HasValue())
		{
			continue;
		}

		const FWaterBodyQueryResult& WaterQuery = QueryResult.GetValue();
		const FVector SurfaceLocation = WaterQuery.GetWaterSurfaceLocation();
		const double DistanceSq = FVector::DistSquared2D(ViewLocation, SurfaceLocation);
		if (DistanceSq >= BestDistanceSq)
		{
			continue;
		}

		BestDistanceSq = DistanceSq;
		OutSurfaceZ = SurfaceLocation.Z;
		OutDepthBelowSurfaceCm = FMath::Max(0.0f, WaterQuery.GetImmersionDepth());
		bFoundWaterBody = true;
	}

	return bFoundWaterBody;
}

void UMyikaUnderwaterPostProcessComponent::ApplyEffectState(bool bShouldEnable, float SurfaceZ, float DepthBelowSurfaceCm)
{
	bEffectActive = bShouldEnable;
	CurrentWaterSurfaceZ = SurfaceZ;
	CurrentDepthBelowSurfaceCm = DepthBelowSurfaceCm;

	if (IsValid(PostProcessComponent))
	{
		PostProcessComponent->Priority = PostProcessPriority;
		PostProcessComponent->BlendWeight = bEffectActive ? 1.0f : 0.0f;
	}

	PushMaterialParameters();
}
