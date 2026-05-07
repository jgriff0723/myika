// Copyright (c) Myika AI. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "MyikaLightningComponent.generated.h"

class UDirectionalLightComponent;
class UMyikaSubsystem;
class USoundBase;
class UStaticMesh;

UCLASS(ClassGroup = "Myika|Sky", meta = (BlueprintSpawnableComponent))
class MYIKASKY_API UMyikaLightningComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UMyikaLightningComponent();

	/** Storm intensity threshold 0..1 that enables automatic lightning strikes. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Myika|Sky|Lightning", meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0", SliderExponent = "2.0"))
	float StormThreshold = 0.5f;

	/** Minimum automatic cooldown between lightning strikes in seconds. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Myika|Sky|Lightning", meta = (ClampMin = "0.0", UIMin = "0.0", UIMax = "30.0"))
	float MinCooldownSeconds = 5.0f;

	/** Maximum automatic cooldown between lightning strikes in seconds. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Myika|Sky|Lightning", meta = (ClampMin = "0.0", UIMin = "0.0", UIMax = "60.0"))
	float MaxCooldownSeconds = 25.0f;

	/** Static mesh used for the visible lightning bolt. Defaults to Engine's cylinder if available. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Myika|Sky|Lightning")
	TObjectPtr<UStaticMesh> BoltMesh = nullptr;

	/** Thunder sound played after the speed-of-sound delay. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Myika|Sky|Lightning")
	TObjectPtr<USoundBase> ThunderSound = nullptr;

	/** Approximate cloud altitude used when choosing a strike location. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Myika|Sky|Lightning", meta = (ClampMin = "0.0", UIMin = "0.0", UIMax = "20000.0"))
	float CloudAltitude = 5000.0f;

	/** Random XY radius around the player used when placing the strike. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Myika|Sky|Lightning", meta = (ClampMin = "0.0", UIMin = "0.0", UIMax = "20000.0"))
	float StrikeRadius = 8000.0f;

	/** Light pulse multiplier applied to the owning sky actor's directional light. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Myika|Sky|Lightning", meta = (ClampMin = "1.0", UIMin = "1.0", UIMax = "8.0"))
	float SunPulseIntensityMultiplier = 3.0f;

	/** Duration in seconds that the sunlight pulse stays at peak brightness. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Myika|Sky|Lightning", meta = (ClampMin = "0.0", UIMin = "0.0", UIMax = "2.0"))
	float FlashHoldSeconds = 0.15f;

	/** Duration in seconds that the lightning flash fades from 1 to 0. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Myika|Sky|Lightning", meta = (ClampMin = "0.0", UIMin = "0.0", UIMax = "3.0"))
	float FlashFadeSeconds = 0.6f;

	/** Lifetime in seconds for the spawned bolt actor. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Myika|Sky|Lightning", meta = (ClampMin = "0.0", UIMin = "0.0", UIMax = "5.0"))
	float BoltLifetimeSeconds = 0.4f;

	/** Speed of sound in m/s used to delay thunder playback. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Myika|Sky|Lightning", meta = (ClampMin = "1.0", UIMin = "1.0", UIMax = "1000.0"))
	float SpeedOfSoundMetersPerSecond = 340.0f;

	/** Remaining cooldown before the next automatic strike can fire. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Myika|Debug")
	float CooldownRemainingSeconds = 0.0f;

	UFUNCTION(BlueprintCallable, Category = "Myika|Sky|Lightning")
	void TriggerStrike();

	UFUNCTION(BlueprintPure, Category = "Myika|Sky|Lightning")
	float GetCooldownRemainingSeconds() const { return CooldownRemainingSeconds; }

	virtual void TickComponent(float DeltaSeconds, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

protected:
	virtual void BeginPlay() override;

private:
	UMyikaSubsystem* ResolveSubsystem() const;
	UDirectionalLightComponent* ResolveDirectionalLight() const;
	void BeginFlash();
	void UpdateFlash(float DeltaSeconds);
	void ApplyLightningFlash(float NewFlashValue);
	FVector GetStrikeOrigin() const;
	FVector GenerateStrikeLocation() const;
	void StartNextCooldown();
	void RestoreDirectionalLight();
	void PlayThunderAtLocation(FVector StrikeLocation);

	mutable TWeakObjectPtr<UMyikaSubsystem> CachedSubsystem;
	mutable TWeakObjectPtr<UDirectionalLightComponent> CachedDirectionalLight;
	float FlashElapsedSeconds = 0.0f;
	float DirectionalLightBaseIntensity = 0.0f;
	bool bFlashActive = false;
	bool bDirectionalPulseActive = false;
};
