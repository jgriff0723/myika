// Copyright (c) Myika AI. All rights reserved.

#include "MyikaLightningComponent.h"

#include "Camera/PlayerCameraManager.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/GameInstance.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
#include "MyikaSubsystem.h"
#include "Sound/SoundBase.h"

DEFINE_LOG_CATEGORY_STATIC(LogMyikaLightning, Log, All);

UMyikaLightningComponent::UMyikaLightningComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	BoltMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
}

void UMyikaLightningComponent::BeginPlay()
{
	Super::BeginPlay();
	StartNextCooldown();
}

void UMyikaLightningComponent::TickComponent(float DeltaSeconds, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaSeconds, TickType, ThisTickFunction);

	if (CooldownRemainingSeconds > 0.0f)
	{
		CooldownRemainingSeconds = FMath::Max(0.0f, CooldownRemainingSeconds - DeltaSeconds);
	}

	if (bFlashActive)
	{
		UpdateFlash(DeltaSeconds);
	}

	if (CooldownRemainingSeconds > 0.0f)
	{
		return;
	}

	if (UMyikaSubsystem* Subsystem = ResolveSubsystem())
	{
		if (Subsystem->GetGlobalState().StormIntensity > StormThreshold)
		{
			TriggerStrike();
		}
	}
}

void UMyikaLightningComponent::TriggerStrike()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	const FVector StrikeLocation = GenerateStrikeLocation();

	BeginFlash();
	StartNextCooldown();

	if (BoltMesh)
	{
		AStaticMeshActor* BoltActor = World->SpawnActor<AStaticMeshActor>(StrikeLocation, FRotator(0.0f, FMath::FRandRange(0.0f, 360.0f), 0.0f));
		if (BoltActor)
		{
			BoltActor->SetLifeSpan(BoltLifetimeSeconds);

			if (UStaticMeshComponent* StaticMeshComponent = BoltActor->GetStaticMeshComponent())
			{
				StaticMeshComponent->SetMobility(EComponentMobility::Movable);
				StaticMeshComponent->SetStaticMesh(BoltMesh);
				StaticMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
				StaticMeshComponent->SetCastShadow(false);
				StaticMeshComponent->SetWorldScale3D(FVector(0.08f, 0.08f, 25.0f));
			}
		}
	}

	if (ThunderSound)
	{
		const FVector ListenerLocation = GetStrikeOrigin();
		const float DistanceMeters = FVector::Distance(StrikeLocation, ListenerLocation) / 100.0f;
		const float ThunderDelaySeconds = FMath::Max(0.0f, DistanceMeters / SpeedOfSoundMetersPerSecond);

		FTimerDelegate ThunderDelegate = FTimerDelegate::CreateUObject(this, &UMyikaLightningComponent::PlayThunderAtLocation, StrikeLocation);
		FTimerHandle ThunderTimerHandle;
		World->GetTimerManager().SetTimer(ThunderTimerHandle, ThunderDelegate, ThunderDelaySeconds, false);
	}
}

UMyikaSubsystem* UMyikaLightningComponent::ResolveSubsystem() const
{
	if (CachedSubsystem.IsValid())
	{
		return CachedSubsystem.Get();
	}

	if (const AActor* Owner = GetOwner())
	{
		if (UGameInstance* GameInstance = Owner->GetGameInstance())
		{
			CachedSubsystem = GameInstance->GetSubsystem<UMyikaSubsystem>();
		}
	}

	return CachedSubsystem.Get();
}

UDirectionalLightComponent* UMyikaLightningComponent::ResolveDirectionalLight() const
{
	if (CachedDirectionalLight.IsValid())
	{
		return CachedDirectionalLight.Get();
	}

	if (const AActor* Owner = GetOwner())
	{
		CachedDirectionalLight = Owner->FindComponentByClass<UDirectionalLightComponent>();
	}

	return CachedDirectionalLight.Get();
}

void UMyikaLightningComponent::BeginFlash()
{
	FlashElapsedSeconds = 0.0f;
	bFlashActive = true;
	ApplyLightningFlash(1.0f);

	if (UDirectionalLightComponent* DirectionalLight = ResolveDirectionalLight())
	{
		if (bDirectionalPulseActive)
		{
			RestoreDirectionalLight();
		}

		DirectionalLightBaseIntensity = DirectionalLight->Intensity;
		DirectionalLight->SetIntensity(DirectionalLightBaseIntensity * SunPulseIntensityMultiplier);
		bDirectionalPulseActive = true;
	}
}

void UMyikaLightningComponent::UpdateFlash(float DeltaSeconds)
{
	FlashElapsedSeconds += DeltaSeconds;

	if (bDirectionalPulseActive && FlashElapsedSeconds >= FlashHoldSeconds)
	{
		RestoreDirectionalLight();
	}

	if (FlashElapsedSeconds <= FlashHoldSeconds)
	{
		ApplyLightningFlash(1.0f);
		return;
	}

	const float FadeElapsedSeconds = FlashElapsedSeconds - FlashHoldSeconds;
	const float FadeAlpha = FlashFadeSeconds <= KINDA_SMALL_NUMBER
		? 0.0f
		: FMath::Clamp(1.0f - (FadeElapsedSeconds / FlashFadeSeconds), 0.0f, 1.0f);

	ApplyLightningFlash(FadeAlpha);

	if (FadeAlpha <= 0.0f)
	{
		bFlashActive = false;
		ApplyLightningFlash(0.0f);
		RestoreDirectionalLight();
	}
}

void UMyikaLightningComponent::ApplyLightningFlash(float NewFlashValue)
{
	if (UMyikaSubsystem* Subsystem = ResolveSubsystem())
	{
		Subsystem->SetLightningFlash(NewFlashValue);
	}
}

FVector UMyikaLightningComponent::GetStrikeOrigin() const
{
	if (APlayerCameraManager* CameraManager = UGameplayStatics::GetPlayerCameraManager(this, 0))
	{
		return CameraManager->GetCameraLocation();
	}

	if (const APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(this, 0))
	{
		return PlayerPawn->GetActorLocation();
	}

	return GetOwner() ? GetOwner()->GetActorLocation() : FVector::ZeroVector;
}

FVector UMyikaLightningComponent::GenerateStrikeLocation() const
{
	const FVector Origin = GetStrikeOrigin();
	const FVector2D Offset2D = FMath::RandPointInCircle(StrikeRadius);
	return FVector(Origin.X + Offset2D.X, Origin.Y + Offset2D.Y, Origin.Z + CloudAltitude);
}

void UMyikaLightningComponent::StartNextCooldown()
{
	const float MinCooldown = FMath::Max(0.0f, MinCooldownSeconds);
	const float MaxCooldown = FMath::Max(MinCooldown, MaxCooldownSeconds);
	CooldownRemainingSeconds = FMath::FRandRange(MinCooldown, MaxCooldown);
}

void UMyikaLightningComponent::RestoreDirectionalLight()
{
	if (bDirectionalPulseActive)
	{
		if (UDirectionalLightComponent* DirectionalLight = ResolveDirectionalLight())
		{
			DirectionalLight->SetIntensity(DirectionalLightBaseIntensity);
		}

		bDirectionalPulseActive = false;
	}
}

void UMyikaLightningComponent::PlayThunderAtLocation(FVector StrikeLocation)
{
	if (ThunderSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, ThunderSound, StrikeLocation);
	}
}
