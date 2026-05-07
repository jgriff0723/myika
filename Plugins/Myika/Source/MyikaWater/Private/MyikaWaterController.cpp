// Copyright (c) Myika AI. All rights reserved.

#include "MyikaWaterController.h"

#include "Components/SceneComponent.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "MyikaSubsystem.h"

DEFINE_LOG_CATEGORY_STATIC(LogMyikaWaterController, Log, All);

AMyikaWaterController::AMyikaWaterController()
{
	PrimaryActorTick.bCanEverTick = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);
}

UMyikaSubsystem* AMyikaWaterController::GetMyikaSubsystem() const
{
	const UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
	}

	UGameInstance* GameInstance = World->GetGameInstance();
	if (!GameInstance)
	{
		return nullptr;
	}

	return GameInstance->GetSubsystem<UMyikaSubsystem>();
}

void AMyikaWaterController::BeginPlay()
{
	Super::BeginPlay();

	if (bLogOnBeginPlay)
	{
		UE_LOG(LogMyikaWaterController, Log, TEXT("MyikaWater controller active: %s"), *GetName());
	}
}
