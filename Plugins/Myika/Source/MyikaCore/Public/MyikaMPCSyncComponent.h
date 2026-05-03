// Copyright (c) Myika AI. All rights reserved.
//
// UMyikaMPCSyncComponent
//
// Bridges the canonical UMyikaSubsystem state into the global MPC_Myika material
// parameter collection so shaders can react to time-of-day and weather changes.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "MyikaTypes.h"
#include "MyikaMPCSyncComponent.generated.h"

class UMaterialParameterCollection;

UCLASS(ClassGroup = (Myika), meta = (BlueprintSpawnableComponent), DisplayName = "Myika MPC Sync Component")
class MYIKACORE_API UMyikaMPCSyncComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UMyikaMPCSyncComponent();

	/** Material Parameter Collection asset that receives the current Myika global state. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Myika|Core")
	TObjectPtr<UMaterialParameterCollection> MPC = nullptr;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;

private:
	UFUNCTION()
	void HandleGlobalStateChanged(const FMyikaGlobalState& NewState);

	void WriteAll(const FMyikaGlobalState& State);
};
