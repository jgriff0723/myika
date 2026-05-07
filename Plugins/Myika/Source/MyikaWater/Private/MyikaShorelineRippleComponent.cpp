// Copyright (c) Myika AI. All rights reserved.

#include "MyikaShorelineRippleComponent.h"

#include "Camera/PlayerCameraManager.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/TextureRenderTarget2D.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Math/NumericLimits.h"
#include "NiagaraComponent.h"
#include "NiagaraSystem.h"
#include "UObject/SoftObjectPath.h"
#include "WaterBodyActor.h"
#include "WaterBodyComponent.h"
#include "WaterBodyTypes.h"

DEFINE_LOG_CATEGORY_STATIC(LogMyikaShorelineRipple, Log, All);

namespace
{
	const TCHAR* DefaultRippleSystemPath = TEXT("/Myika/MyikaWater/Niagara/NS_MyikaShorelineRipple.NS_MyikaShorelineRipple");
	const TCHAR* DefaultRippleRenderTargetPath = TEXT("/Myika/MyikaWater/Materials/RT_MyikaShorelineRipple.RT_MyikaShorelineRipple");
}

UMyikaShorelineRippleComponent::UMyikaShorelineRippleComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickGroup = TG_PostPhysics;

	DefaultRippleSystemAsset = TSoftObjectPtr<UNiagaraSystem>(FSoftObjectPath(DefaultRippleSystemPath));
	DefaultRippleNormalRenderTargetAsset = TSoftObjectPtr<UTextureRenderTarget2D>(FSoftObjectPath(DefaultRippleRenderTargetPath));

	InteractionSocketNames =
	{
		TEXT("foot_l"),
		TEXT("foot_r"),
		TEXT("hand_l"),
		TEXT("hand_r")
	};
}

void UMyikaShorelineRippleComponent::BeginPlay()
{
	Super::BeginPlay();

	ResolveDefaultAssets();
	EnsureNiagaraComponent();
	PushNiagaraBindings();
}

void UMyikaShorelineRippleComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (NiagaraComponent != nullptr)
	{
		NiagaraComponent->DestroyComponent();
		NiagaraComponent = nullptr;
	}

	Super::EndPlay(EndPlayReason);
}

void UMyikaShorelineRippleComponent::TickComponent(
	float DeltaTime,
	ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	EnsureNiagaraComponent();

	if (bAutoTrackWaterBody)
	{
		RefreshTrackedWaterBody();
	}

	CurrentDomainCenter = ResolveDomainCenter();
	PushNiagaraBindings();
	UpdateSimulationState(DeltaTime);
}

void UMyikaShorelineRippleComponent::InjectRippleAtWorldLocation(FVector WorldLocation, float RadiusCm, float Strength)
{
	EnsureNiagaraComponent();
	if (NiagaraComponent == nullptr)
	{
		return;
	}

	RippleSequence += 1.0f;

	const float FinalRadius = RadiusCm > 0.0f ? RadiusCm : DefaultRippleRadiusCm;
	const float FinalStrength = Strength >= 0.0f ? Strength : DefaultRippleStrength;

	NiagaraComponent->SetVariableVec3(InjectionPointParameterName, WorldLocation);
	NiagaraComponent->SetVariableFloat(InjectionRadiusParameterName, FinalRadius);
	NiagaraComponent->SetVariableFloat(InjectionStrengthParameterName, FinalStrength);
	NiagaraComponent->SetVariableFloat(InjectionSequenceParameterName, RippleSequence);
}

void UMyikaShorelineRippleComponent::SetTrackedWaterBody(AWaterBody* InWaterBody)
{
	TrackedWaterBody = InWaterBody;
}

void UMyikaShorelineRippleComponent::ResolveDefaultAssets()
{
	if ((RippleSystem == nullptr) && !DefaultRippleSystemAsset.IsNull())
	{
		RippleSystem = DefaultRippleSystemAsset.LoadSynchronous();
	}

	if ((RippleNormalRenderTarget == nullptr) && !DefaultRippleNormalRenderTargetAsset.IsNull())
	{
		RippleNormalRenderTarget = DefaultRippleNormalRenderTargetAsset.LoadSynchronous();
	}
}

void UMyikaShorelineRippleComponent::EnsureNiagaraComponent()
{
	ResolveDefaultAssets();

	if (RippleSystem == nullptr || GetOwner() == nullptr)
	{
		return;
	}

	if (NiagaraComponent == nullptr)
	{
		NiagaraComponent = NewObject<UNiagaraComponent>(GetOwner(), TEXT("MyikaShorelineRippleNiagara"));
		NiagaraComponent->SetupAttachment(GetOwner()->GetRootComponent());
		NiagaraComponent->SetAutoActivate(false);
		NiagaraComponent->RegisterComponent();
	}

	if (NiagaraComponent->GetAsset() != RippleSystem)
	{
		NiagaraComponent->SetAsset(RippleSystem);
	}
}

void UMyikaShorelineRippleComponent::PushNiagaraBindings() const
{
	if (NiagaraComponent == nullptr)
	{
		return;
	}

	NiagaraComponent->SetWorldLocation(CurrentDomainCenter);
	NiagaraComponent->SetVariableVec3(DomainCenterParameterName, CurrentDomainCenter);
	NiagaraComponent->SetVariableFloat(DomainRadiusParameterName, SimulationRadiusCm);
	NiagaraComponent->SetVariableBool(SimulationEnabledParameterName, bNiagaraActive);

	if (RippleNormalRenderTarget != nullptr)
	{
		NiagaraComponent->SetVariableTextureRenderTarget(RippleNormalRenderTargetParameterName, RippleNormalRenderTarget);
	}
}

void UMyikaShorelineRippleComponent::RefreshTrackedWaterBody()
{
	AActor* Owner = GetOwner();
	if (Owner == nullptr)
	{
		TrackedWaterBody = nullptr;
		return;
	}

	TArray<AActor*> OverlappingActors;
	Owner->GetOverlappingActors(OverlappingActors, AWaterBody::StaticClass());

	if (OverlappingActors.IsEmpty())
	{
		TrackedWaterBody = nullptr;
		return;
	}

	const FVector OwnerLocation = Owner->GetActorLocation();
	AWaterBody* BestWaterBody = nullptr;
	float BestDistanceSquared = TNumericLimits<float>::Max();

	for (AActor* Candidate : OverlappingActors)
	{
		AWaterBody* WaterBody = Cast<AWaterBody>(Candidate);
		if (WaterBody == nullptr || WaterBody->GetWaterBodyComponent() == nullptr)
		{
			continue;
		}

		const float DistanceSquared = FVector::DistSquaredXY(OwnerLocation, WaterBody->GetActorLocation());
		if (DistanceSquared < BestDistanceSquared)
		{
			BestDistanceSquared = DistanceSquared;
			BestWaterBody = WaterBody;
		}
	}

	TrackedWaterBody = BestWaterBody;
}

void UMyikaShorelineRippleComponent::UpdateSimulationState(float DeltaTime)
{
	if (NiagaraComponent == nullptr || GetOwner() == nullptr)
	{
		return;
	}

	FVector WaterSurfaceLocation = FVector::ZeroVector;
	float WaterDepth = 0.0f;
	const bool bNearWater = QueryWaterAtLocation(GetOwner()->GetActorLocation(), WaterSurfaceLocation, WaterDepth);
	const bool bShouldRunSimulation = !bOnlySimulateNearWater || bNearWater;

	ApplyNiagaraActivation(bShouldRunSimulation);
	PushNiagaraBindings();

	if (!bShouldRunSimulation || !bAutoInjectFromOwnerMotion)
	{
		return;
	}

	const float OwnerSpeed = GetOwner()->GetVelocity().Size();
	if (OwnerSpeed < MinimumInjectionSpeedCmPerSecond)
	{
		return;
	}

	TimeUntilNextInjection = FMath::Max(0.0f, TimeUntilNextInjection - DeltaTime);
	if (TimeUntilNextInjection > 0.0f)
	{
		return;
	}

	TimeUntilNextInjection = InjectionIntervalSeconds;

	for (const FVector& InteractionPoint : CollectInteractionPoints())
	{
		FVector SampledSurfaceLocation = FVector::ZeroVector;
		float SampledDepth = 0.0f;
		if (!QueryWaterAtLocation(InteractionPoint, SampledSurfaceLocation, SampledDepth))
		{
			continue;
		}

		if (InteractionPoint.Z <= SampledSurfaceLocation.Z + MaxInteractionHeightAboveSurfaceCm)
		{
			InjectRippleAtWorldLocation(SampledSurfaceLocation);
			return;
		}
	}
}

void UMyikaShorelineRippleComponent::ApplyNiagaraActivation(bool bShouldBeActive)
{
	if (NiagaraComponent == nullptr || bNiagaraActive == bShouldBeActive)
	{
		bNiagaraActive = bShouldBeActive;
		return;
	}

	bNiagaraActive = bShouldBeActive;

	if (bShouldBeActive)
	{
		NiagaraComponent->Activate();
	}
	else
	{
		NiagaraComponent->Deactivate();
	}
}

FVector UMyikaShorelineRippleComponent::ResolveDomainCenter() const
{
	AActor* Owner = GetOwner();
	if (Owner == nullptr)
	{
		return FVector::ZeroVector;
	}

	if (const APawn* Pawn = Cast<APawn>(Owner))
	{
		if (const APlayerController* PlayerController = Cast<APlayerController>(Pawn->GetController()))
		{
			if (const APlayerCameraManager* CameraManager = PlayerController->PlayerCameraManager)
			{
				return CameraManager->GetCameraLocation();
			}
		}
	}

	return Owner->GetActorLocation();
}

TArray<FVector> UMyikaShorelineRippleComponent::CollectInteractionPoints() const
{
	TArray<FVector> Result;

	AActor* Owner = GetOwner();
	if (Owner == nullptr)
	{
		return Result;
	}

	Result.Add(Owner->GetActorLocation());

	const USkeletalMeshComponent* SkeletalMeshComponent = Owner->FindComponentByClass<USkeletalMeshComponent>();
	if (SkeletalMeshComponent == nullptr)
	{
		return Result;
	}

	for (const FName& SocketName : InteractionSocketNames)
	{
		if (SocketName.IsNone() || !SkeletalMeshComponent->DoesSocketExist(SocketName))
		{
			continue;
		}

		Result.Add(SkeletalMeshComponent->GetSocketLocation(SocketName));
	}

	return Result;
}

bool UMyikaShorelineRippleComponent::QueryWaterAtLocation(
	const FVector& WorldLocation,
	FVector& OutWaterSurfaceLocation,
	float& OutWaterDepth) const
{
	const AWaterBody* WaterBody = TrackedWaterBody.Get();
	if (WaterBody == nullptr || WaterBody->GetWaterBodyComponent() == nullptr)
	{
		return false;
	}

	const TValueOrError<FWaterBodyQueryResult, EWaterBodyQueryError> QueryResult =
		WaterBody->GetWaterBodyComponent()->TryQueryWaterInfoClosestToWorldLocation(
			WorldLocation,
			EWaterBodyQueryFlags::ComputeLocation |
			EWaterBodyQueryFlags::ComputeDepth |
			EWaterBodyQueryFlags::IncludeWaves);

	if (!QueryResult.HasValue())
	{
		return false;
	}

	const FWaterBodyQueryResult& Query = QueryResult.GetValue();
	OutWaterSurfaceLocation = Query.GetWaterSurfaceLocation();
	OutWaterDepth = Query.GetWaterSurfaceDepth();
	return true;
}
