// Copyright (c) Myika AI. All rights reserved.
//
// AMyikaCelestialActor renders the simple V1 night-sky layer: stars, moon, and moonlight.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "UObject/SoftObjectPtr.h"
#include "MyikaCelestialActor.generated.h"

class UDirectionalLightComponent;
class UMaterialInstanceDynamic;
class UMaterialInterface;
class USceneComponent;
class UStaticMeshComponent;

UCLASS(BlueprintType, Blueprintable, ClassGroup=(Myika))
class MYIKASKY_API AMyikaCelestialActor : public AActor
{
	GENERATED_BODY()

public:
	AMyikaCelestialActor();

	virtual void OnConstruction(const FTransform& Transform) override;

	/** Apply the current moon direction and fade alpha computed by the owning sky actor. */
	UFUNCTION(BlueprintCallable, Category = "Myika|Sky|Stars")
	void ApplyCelestialState(const FVector& InMoonDirection, float InVisibilityAlpha, bool bShouldRenderCelestials);

	/** Current fade alpha used to render the night sky. */
	UFUNCTION(BlueprintPure, Category = "Myika|Sky|Debug")
	float GetCurrentVisibilityAlpha() const { return CurrentVisibilityAlpha; }

	/** Current moonlight direction written by the owning sky actor. */
	UFUNCTION(BlueprintPure, Category = "Myika|Sky|Debug")
	FVector GetCurrentMoonDirection() const { return CurrentMoonDirection; }

	/** Root anchor for all celestial components. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Myika|Sky|Stars")
	TObjectPtr<USceneComponent> Root;

	/** Large inverted sphere used for the star field. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Myika|Sky|Stars")
	TObjectPtr<UStaticMeshComponent> StarsSphere;

	/** Small sphere representing the moon body. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Myika|Sky|Stars")
	TObjectPtr<UStaticMeshComponent> MoonMesh;

	/** Secondary atmosphere light that provides faint moonlight at night. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Myika|Sky|Stars")
	TObjectPtr<UDirectionalLightComponent> MoonLight;

	/** Material asset for the procedural star field. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Myika|Sky|Stars", meta = (AllowedClasses = "/Script/Engine.MaterialInterface"))
	TSoftObjectPtr<UMaterialInterface> StarsMaterialAsset;

	/** Material asset for the moon body. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Myika|Sky|Stars", meta = (AllowedClasses = "/Script/Engine.MaterialInterface"))
	TSoftObjectPtr<UMaterialInterface> MoonMaterialAsset;

	/** Radius of the star sphere in world units. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Myika|Sky|Stars",
		meta = (ClampMin = "100000.0", UIMin = "100000.0", UIMax = "250000.0"))
	float StarsSphereRadius = 125000.0f;

	/** Distance from the actor origin to the moon body in world units. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Myika|Sky|Stars",
		meta = (ClampMin = "50000.0", UIMin = "50000.0", UIMax = "150000.0"))
	float MoonOrbitDistance = 110000.0f;

	/** Radius of the moon body mesh in world units. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Myika|Sky|Stars",
		meta = (ClampMin = "250.0", UIMin = "250.0", UIMax = "5000.0"))
	float MoonRadius = 1500.0f;

	/** Max moonlight intensity when the sky is fully in night mode. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Myika|Sky|Stars",
		meta = (ClampMin = "0.0", ClampMax = "5.0", UIMin = "0.0", UIMax = "1.0", SliderExponent = "2.0"))
	float MoonLightMaxIntensity = 0.2f;

	/** Slight blue tint for nighttime moonlight. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Myika|Sky|Stars")
	FLinearColor MoonLightColor = FLinearColor(0.55f, 0.65f, 1.0f);

private:
	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> StarsMaterialInstance;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> MoonMaterialInstance;

	UPROPERTY(Transient)
	float CurrentVisibilityAlpha = 0.0f;

	UPROPERTY(Transient)
	FVector CurrentMoonDirection = FVector(0.0f, 0.0f, -1.0f);

	void ApplyScaleSettings();
	void RefreshAssetBindings();
};
