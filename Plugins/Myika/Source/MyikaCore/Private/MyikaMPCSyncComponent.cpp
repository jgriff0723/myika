// Copyright (c) Myika AI. All rights reserved.

#include "MyikaMPCSyncComponent.h"

#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Materials/MaterialParameterCollection.h"
#include "Materials/MaterialParameterCollectionInstance.h"
#include "Misc/PackageName.h"
#include "MyikaSubsystem.h"
#include "UObject/SoftObjectPath.h"

DEFINE_LOG_CATEGORY_STATIC(LogMyikaMPCSyncComponent, Log, All);

namespace
{
	const FName TimeOfDayName(TEXT("TimeOfDay"));
	const FName WetnessName(TEXT("Wetness"));
	const FName SnowCoverageName(TEXT("SnowCoverage"));
	const FName TemperatureName(TEXT("Temperature"));
	const FName LightningFlashName(TEXT("LightningFlash"));
	const FName StormIntensityName(TEXT("StormIntensity"));
	const FName WaterLevelName(TEXT("WaterLevel"));
	const FName SunDirectionName(TEXT("SunDirection"));
	const FName MoonDirectionName(TEXT("MoonDirection"));
	const FName WindVectorName(TEXT("WindVector"));
	const FString DefaultMPCPackagePath(TEXT("/Myika/MyikaCore/MPC_Myika"));
	const FSoftObjectPath DefaultMPCObjectPath(TEXT("/Myika/MyikaCore/MPC_Myika.MPC_Myika"));

	UMyikaSubsystem* ResolveMyikaSubsystem(const UActorComponent* Component)
	{
		if (!IsValid(Component))
		{
			return nullptr;
		}

		const UWorld* World = Component->GetWorld();
		if (!IsValid(World))
		{
			return nullptr;
		}

		UGameInstance* GameInstance = World->GetGameInstance();
		if (!IsValid(GameInstance))
		{
			return nullptr;
		}

		return GameInstance->GetSubsystem<UMyikaSubsystem>();
	}
}

UMyikaMPCSyncComponent::UMyikaMPCSyncComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UMyikaMPCSyncComponent::BeginPlay()
{
	Super::BeginPlay();

	if (!IsValid(MPC) && FPackageName::DoesPackageExist(DefaultMPCPackagePath))
	{
		MPC = Cast<UMaterialParameterCollection>(DefaultMPCObjectPath.TryLoad());
	}

	UMyikaSubsystem* Subsystem = ResolveMyikaSubsystem(this);
	if (!IsValid(Subsystem))
	{
		UE_LOG(
			LogMyikaMPCSyncComponent,
			Warning,
			TEXT("%s could not resolve UMyikaSubsystem during BeginPlay."),
			*GetNameSafe(this));
		return;
	}

	Subsystem->OnGlobalStateChanged.RemoveDynamic(this, &UMyikaMPCSyncComponent::HandleGlobalStateChanged);
	Subsystem->OnGlobalStateChanged.AddDynamic(this, &UMyikaMPCSyncComponent::HandleGlobalStateChanged);
	WriteAll(Subsystem->GetGlobalState());
}

void UMyikaMPCSyncComponent::EndPlay(const EEndPlayReason::Type Reason)
{
	if (UMyikaSubsystem* Subsystem = ResolveMyikaSubsystem(this))
	{
		Subsystem->OnGlobalStateChanged.RemoveDynamic(this, &UMyikaMPCSyncComponent::HandleGlobalStateChanged);
	}

	Super::EndPlay(Reason);
}

void UMyikaMPCSyncComponent::HandleGlobalStateChanged(const FMyikaGlobalState& NewState)
{
	WriteAll(NewState);
}

void UMyikaMPCSyncComponent::WriteAll(const FMyikaGlobalState& State)
{
	UWorld* World = GetWorld();
	if (!IsValid(World))
	{
		UE_LOG(
			LogMyikaMPCSyncComponent,
			Warning,
			TEXT("%s has no valid world while writing MPC state."),
			*GetNameSafe(this));
		return;
	}

	if (!IsValid(MPC))
	{
		UE_LOG(
			LogMyikaMPCSyncComponent,
			Warning,
			TEXT("%s has no Material Parameter Collection assigned; skipping sync."),
			*GetNameSafe(this));
		return;
	}

	UMaterialParameterCollectionInstance* MPCInstance = World->GetParameterCollectionInstance(MPC);
	if (!IsValid(MPCInstance))
	{
		UE_LOG(
			LogMyikaMPCSyncComponent,
			Warning,
			TEXT("%s could not resolve a live MPC instance for %s; skipping sync."),
			*GetNameSafe(this),
			*GetNameSafe(MPC));
		return;
	}

	const auto SetScalar = [this, MPCInstance](const FName ParameterName, const float Value)
	{
		if (!MPCInstance->SetScalarParameterValue(ParameterName, Value))
		{
			UE_LOG(
				LogMyikaMPCSyncComponent,
				Warning,
				TEXT("%s failed to set scalar MPC parameter '%s'."),
				*GetNameSafe(this),
				*ParameterName.ToString());
		}
	};

	const auto SetVector = [this, MPCInstance](const FName ParameterName, const FVector& Value)
	{
		if (!MPCInstance->SetVectorParameterValue(
				ParameterName,
				FLinearColor(Value.X, Value.Y, Value.Z, 0.f)))
		{
			UE_LOG(
				LogMyikaMPCSyncComponent,
				Warning,
				TEXT("%s failed to set vector MPC parameter '%s'."),
				*GetNameSafe(this),
				*ParameterName.ToString());
		}
	};

	SetScalar(TimeOfDayName, State.TimeOfDay);
	SetScalar(WetnessName, State.Wetness);
	SetScalar(SnowCoverageName, State.SnowCoverage);
	SetScalar(TemperatureName, State.Temperature);
	SetScalar(LightningFlashName, State.LightningFlash);
	SetScalar(StormIntensityName, State.StormIntensity);
	SetScalar(WaterLevelName, State.WaterLevel);
	SetVector(SunDirectionName, State.SunDirection);
	SetVector(MoonDirectionName, State.MoonDirection);
	SetVector(WindVectorName, State.WindVector);
}
