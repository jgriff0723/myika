// Copyright (c) Myika AI. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MyikaWaterController.generated.h"

class UMyikaSubsystem;
class USceneComponent;

UCLASS(BlueprintType, Blueprintable, ClassGroup=(Myika), meta=(DisplayName="Myika Water Controller"))
class MYIKAWATER_API AMyikaWaterController : public AActor
{
	GENERATED_BODY()

public:
	AMyikaWaterController();

	/** Resolve the shared Myika subsystem for later water slices. */
	UFUNCTION(BlueprintPure, Category = "Myika|Water")
	UMyikaSubsystem* GetMyikaSubsystem() const;

protected:
	virtual void BeginPlay() override;

private:
	/** Root scene component for the drop-in water controller actor. */
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Myika|Water", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USceneComponent> SceneRoot;

	/** If true, logs a startup breadcrumb when the placeholder water controller begins play. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Myika|Water|Debug", meta = (AllowPrivateAccess = "true"))
	bool bLogOnBeginPlay = true;
};
