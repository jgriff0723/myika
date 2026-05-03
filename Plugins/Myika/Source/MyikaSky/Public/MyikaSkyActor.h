// Copyright (c) Myika AI. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MyikaSkyActor.generated.h"

class UCurveFloat;
class UCurveLinearColor;
class UCurveLinearColorAtlas;
class UDirectionalLightComponent;
class UExponentialHeightFogComponent;
class UMaterialInstanceDynamic;
class UMaterialInterface;
class UMyikaCloudPreset;
class UMyikaLightningComponent;
class UMyikaMPCSyncComponent;
class UPostProcessComponent;
class USceneComponent;
class USkyAtmosphereComponent;
class USkyLightComponent;
class UVolumetricCloudComponent;

USTRUCT(BlueprintType)
struct MYIKASKY_API FMyikaCloudPresetValues
{
	GENERATED_BODY()

	/** Overall cloud density 0..1. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Myika|Sky|Clouds", meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0", SliderExponent = "2.0"))
	float Coverage = 0.4f;

	/** Cloud-shape blend from wispy to puffy. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Myika|Sky|Clouds", meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0", SliderExponent = "2.0"))
	float Morphology = 0.5f;

	/** Wind-driven cloud animation speed multiplier. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Myika|Sky|Clouds", meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0", SliderExponent = "2.0"))
	float WindStrength = 0.1f;

	/** Light absorption through the cloud volume. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Myika|Sky|Clouds", meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0", SliderExponent = "2.0"))
	float Extinction = 0.5f;

	/** High-frequency noise contribution. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Myika|Sky|Clouds", meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0", SliderExponent = "2.0"))
	float Detail = 0.5f;

	/** World-space advection direction for cloud motion. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Myika|Sky|Clouds", meta = (MakeEditWidget = "true"))
	FVector WindDirection = FVector(1.0f, 0.0f, 0.0f);

	/** Base tint applied to the cloud material. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Myika|Sky|Clouds")
	FLinearColor BaseColor = FLinearColor::White;

	void CopyFromPreset(const UMyikaCloudPreset& Preset);
};

UCLASS(BlueprintType, Blueprintable, ClassGroup = (Myika), meta = (DisplayName = "Myika Sky Actor"))
class MYIKASKY_API AMyikaSkyActor : public AActor
{
	GENERATED_BODY()

public:
	AMyikaSkyActor();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual bool ShouldTickIfViewportsOnly() const override;

	/** Shared root for the sky-stack components. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Myika|Sky|Components")
	TObjectPtr<USceneComponent> SceneRoot;

	/** Sky atmosphere body used for aerial scattering and sun-disk rendering. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Myika|Sky|Components")
	TObjectPtr<USkyAtmosphereComponent> SkyAtmosphereComponent;

	/** Dominant movable sun light driven by the global time-of-day state. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Myika|Sky|Components")
	TObjectPtr<UDirectionalLightComponent> SunLightComponent;

	/** Real-time skylight capture that keeps ambient lighting in sync with the sky stack. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Myika|Sky|Components")
	TObjectPtr<USkyLightComponent> SkyLightComponent;

	/** Height fog component that provides the baseline volumetric haze layer. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Myika|Sky|Components")
	TObjectPtr<UExponentialHeightFogComponent> HeightFogComponent;

	/** Volumetric cloud layer that receives the Myika cloud material instance. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Myika|Sky|Components")
	TObjectPtr<UVolumetricCloudComponent> VolumetricCloudComponent;

	/** Unbound post-process volume used for curve-driven exposure compensation. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Myika|Sky|Components")
	TObjectPtr<UPostProcessComponent> PostProcessComponent;

	/** Subsystem-to-MPC bridge for materials that sample MPC_Myika. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Myika|Sky|Components")
	TObjectPtr<UMyikaMPCSyncComponent> MPCSyncComponent;

	/** Lightning helper component used by the future storm-timeline path. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Myika|Sky|Components")
	TObjectPtr<UMyikaLightningComponent> LightningComponent;

	/** Preview time-of-day used before a MyikaSubsystem exists or when scrubbing in editor. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Myika|Sky|Time", meta = (ClampMin = "0.0", ClampMax = "24.0", UIMin = "0.0", UIMax = "24.0"))
	float PreviewTimeOfDay = 12.0f;

	/** Default cloud material instance applied during construction when the asset exists. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Myika|Sky|Clouds", meta = (AllowedClasses = "/Script/Engine.MaterialInterface"))
	TSoftObjectPtr<UMaterialInterface> DefaultCloudMaterial;

	/** Optional curve atlas asset that groups the sky color curves for shared material use later. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Myika|Sky|Curves")
	TObjectPtr<UCurveLinearColorAtlas> CurveAtlas;

	/** Sun light color across the 24-hour cycle. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Myika|Sky|Curves")
	TObjectPtr<UCurveLinearColor> SunColorCurve;

	/** Sky luminance tint across the 24-hour cycle. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Myika|Sky|Curves")
	TObjectPtr<UCurveLinearColor> SkyLuminanceCurve;

	/** Fog tint across the 24-hour cycle. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Myika|Sky|Curves")
	TObjectPtr<UCurveLinearColor> FogColorCurve;

	/** Sun intensity in lux across the 24-hour cycle. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Myika|Sky|Curves", meta = (UIMin = "0.0", UIMax = "150000.0"))
	TObjectPtr<UCurveFloat> SunIntensityCurve;

	/** Fog density across the 24-hour cycle. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Myika|Sky|Curves")
	TObjectPtr<UCurveFloat> FogDensityCurve;

	/** Exposure compensation bias across the 24-hour cycle. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Myika|Sky|Curves")
	TObjectPtr<UCurveFloat> AutoExposureCurve;

	/** Enables real-world solar math through the built-in SunPosition plugin. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Myika|Sky|Sun")
	bool bAccurateMode = false;

	/** Starting UTC datetime used by accurate mode; time-of-day offsets are applied around noon. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Myika|Sky|Sun")
	FDateTime StartingDateUTC;

	/** Latitude in degrees used by accurate-mode solar calculation. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Myika|Sky|Sun", meta = (ClampMin = "-90.0", ClampMax = "90.0", UIMin = "-90.0", UIMax = "90.0"))
	float Latitude = 40.7128f;

	/** Longitude in degrees used by accurate-mode solar calculation. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Myika|Sky|Sun", meta = (ClampMin = "-180.0", ClampMax = "180.0", UIMin = "-180.0", UIMax = "180.0"))
	float Longitude = -74.0060f;

	/** Timezone offset in hours used by the SunPosition plugin. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Myika|Sky|Sun", meta = (ClampMin = "-12.0", ClampMax = "14.0", UIMin = "-12.0", UIMax = "14.0"))
	float TimeZoneOffsetHours = -4.0f;

	/** Last resolved solar altitude in degrees above the horizon. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Myika|Debug")
	float CurrentSolarAltitudeDegrees = 0.0f;

	/** Last resolved solar azimuth in degrees clockwise from north. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Myika|Debug")
	float CurrentSolarAzimuthDegrees = 0.0f;

	/** Last resolved UTC datetime label used by accurate mode. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Myika|Debug")
	FString CurrentUTCDateTimeLabel;

	/** Current cloud preset values applied to the runtime material instance. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Myika|Sky|Clouds")
	FMyikaCloudPresetValues CurrentPresetValues;

	/** Reapplies the latest subsystem or preview state immediately. */
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Myika|Debug")
	void RefreshSky();

	/** Sets the preview time-of-day used by editor tools outside PIE. */
	UFUNCTION(BlueprintCallable, Category = "Myika|Sky|Time")
	void SetPreviewTimeOfDayValue(float InPreviewTimeOfDay);

	/** Returns the last resolved preview or subsystem time-of-day in hours. */
	UFUNCTION(BlueprintPure, Category = "Myika|Sky|Time")
	float GetPreviewTimeOfDayValue() const;

	/** Enables or disables real-world solar math. */
	UFUNCTION(BlueprintCallable, Category = "Myika|Sky|Sun")
	void SetAccurateModeEnabled(bool bInAccurateMode);

	/** Sets the starting UTC datetime used by accurate mode. */
	UFUNCTION(BlueprintCallable, Category = "Myika|Sky|Sun")
	void SetStartingDateUTCValue(FDateTime InStartingDateUTC);

	/** Sets the latitude used by accurate mode. */
	UFUNCTION(BlueprintCallable, Category = "Myika|Sky|Sun")
	void SetLatitudeValue(float InLatitude);

	/** Sets the longitude used by accurate mode. */
	UFUNCTION(BlueprintCallable, Category = "Myika|Sky|Sun")
	void SetLongitudeValue(float InLongitude);

	/** Sets the timezone offset used by accurate mode. */
	UFUNCTION(BlueprintCallable, Category = "Myika|Sky|Sun")
	void SetTimeZoneOffsetHoursValue(float InTimeZoneOffsetHours);

	/** Returns the latest accurate-mode UTC label. */
	UFUNCTION(BlueprintPure, Category = "Myika|Debug")
	const FString& GetCurrentUTCDateTimeLabel() const;

	/** Returns the latest resolved solar altitude in degrees. */
	UFUNCTION(BlueprintPure, Category = "Myika|Debug")
	float GetCurrentSolarAltitudeDegrees() const;

	/** Returns the latest resolved solar azimuth in degrees. */
	UFUNCTION(BlueprintPure, Category = "Myika|Debug")
	float GetCurrentSolarAzimuthDegrees() const;

	/** Lerp the cloud material parameters toward a preset. */
	UFUNCTION(BlueprintCallable, Category = "Myika|Sky|Clouds", meta = (DisplayName = "Lerp To Cloud Preset"))
	void LerpToPreset(UMyikaCloudPreset* TargetPreset, float DurationSeconds);

	/** Triggers a lightning flash immediately. */
	UFUNCTION(BlueprintCallable, Category = "Myika|Sky|Lightning")
	void TriggerLightningNow();

	/** Returns the remaining lightning cooldown time in seconds. */
	UFUNCTION(BlueprintPure, Category = "Myika|Sky|Lightning")
	float GetLightningCooldownRemaining() const;

protected:
	void ApplyConfiguredCloudMaterial();
	void ApplyCloudPresetValues(const FMyikaCloudPresetValues& Values);
	void AdvanceCloudLerp(float CurrentTimeSeconds);
	bool EnsureCloudMaterialInstance();
	void UpdateSunFromSubsystem();
	void ApplyLinearSunRotation(float TimeOfDayHours);
	void ApplyAccurateSunRotation(float TimeOfDayHours);
	FDateTime ResolveCurrentDateTimeUTC(float TimeOfDayHours) const;

private:
	TObjectPtr<UMaterialInstanceDynamic> CloudMaterialInstance = nullptr;
	FMyikaCloudPresetValues StartingPresetValues;
	FMyikaCloudPresetValues TargetPresetValues;
	bool bCloudLerpActive = false;
	float LerpStartTimeSeconds = 0.0f;
	float LerpDurationSeconds = 0.0f;
};
