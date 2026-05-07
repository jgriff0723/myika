// Copyright (c) Myika AI. All rights reserved.

#include "MyikaSkyActor.h"

#include "Components/DirectionalLightComponent.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Components/PostProcessComponent.h"
#include "Components/SceneComponent.h"
#include "Components/SkyAtmosphereComponent.h"
#include "Components/SkyLightComponent.h"
#include "Components/VolumetricCloudComponent.h"
#include "Curves/CurveFloat.h"
#include "Curves/CurveLinearColor.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "MyikaCloudPreset.h"
#include "MyikaLightningComponent.h"
#include "MyikaMPCSyncComponent.h"
#include "MyikaSubsystem.h"

DEFINE_LOG_CATEGORY_STATIC(LogMyikaSky, Log, All);

namespace
{
	const FName NAME_Coverage(TEXT("Coverage"));
	const FName NAME_Morphology(TEXT("Morphology"));
	const FName NAME_WindStrength(TEXT("WindStrength"));
	const FName NAME_Extinction(TEXT("Extinction"));
	const FName NAME_Detail(TEXT("Detail"));
	const FName NAME_WindDirection(TEXT("WindDirection"));
	const FName NAME_BaseColor(TEXT("BaseColor"));
}

void FMyikaCloudPresetValues::CopyFromPreset(const UMyikaCloudPreset& Preset)
{
	Coverage = Preset.Coverage;
	Morphology = Preset.Morphology;
	WindStrength = Preset.WindStrength;
	Extinction = Preset.Extinction;
	Detail = Preset.Detail;
	WindDirection = Preset.WindDirection;
	BaseColor = Preset.BaseColor;
}

AMyikaSkyActor::AMyikaSkyActor()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	SkyAtmosphereComponent = CreateDefaultSubobject<USkyAtmosphereComponent>(TEXT("SkyAtmosphere"));
	SkyAtmosphereComponent->SetupAttachment(SceneRoot);

	SunLightComponent = CreateDefaultSubobject<UDirectionalLightComponent>(TEXT("SunLight"));
	SunLightComponent->SetupAttachment(SceneRoot);
	SunLightComponent->SetMobility(EComponentMobility::Movable);
	SunLightComponent->SetAtmosphereSunLight(true);
	SunLightComponent->SetAtmosphereSunLightIndex(0);
	SunLightComponent->SetIntensity(100000.0f);
	SunLightComponent->SetRelativeRotation(FRotator(-45.0f, 0.0f, 0.0f));

	SkyLightComponent = CreateDefaultSubobject<USkyLightComponent>(TEXT("SkyLight"));
	SkyLightComponent->SetupAttachment(SceneRoot);
	SkyLightComponent->SetMobility(EComponentMobility::Movable);
	SkyLightComponent->SetRealTimeCapture(true);

	HeightFogComponent = CreateDefaultSubobject<UExponentialHeightFogComponent>(TEXT("HeightFog"));
	HeightFogComponent->SetupAttachment(SceneRoot);
	HeightFogComponent->SetMobility(EComponentMobility::Movable);
	HeightFogComponent->bEnableVolumetricFog = true;

	VolumetricCloudComponent = CreateDefaultSubobject<UVolumetricCloudComponent>(TEXT("VolumetricCloud"));
	VolumetricCloudComponent->SetupAttachment(SceneRoot);
	VolumetricCloudComponent->SetMobility(EComponentMobility::Movable);

	PostProcessComponent = CreateDefaultSubobject<UPostProcessComponent>(TEXT("PostProcess"));
	PostProcessComponent->SetupAttachment(SceneRoot);
	PostProcessComponent->bUnbound = true;

	MPCSyncComponent = CreateDefaultSubobject<UMyikaMPCSyncComponent>(TEXT("MPCSync"));
	LightningComponent = CreateDefaultSubobject<UMyikaLightningComponent>(TEXT("Lightning"));

	DefaultCloudMaterial = TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath(TEXT("/Myika/MyikaSky/Materials/MI_MyikaCloud_Default.MI_MyikaCloud_Default")));
	StartingDateUTC = FDateTime(2026, 5, 3, 12, 0, 0);
	StartingPresetValues = CurrentPresetValues;
	TargetPresetValues = CurrentPresetValues;
}

void AMyikaSkyActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	ApplyConfiguredCloudMaterial();
	EnsureCloudMaterialInstance();
	ApplyCloudPresetValues(CurrentPresetValues);
	UpdateSunFromSubsystem();
}

void AMyikaSkyActor::BeginPlay()
{
	Super::BeginPlay();

	ApplyConfiguredCloudMaterial();
	EnsureCloudMaterialInstance();
	ApplyCloudPresetValues(CurrentPresetValues);
	UpdateSunFromSubsystem();
}

void AMyikaSkyActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	UpdateSunFromSubsystem();

	if (bCloudLerpActive)
	{
		const float CurrentTimeSeconds = GetWorld() ? GetWorld()->GetTimeSeconds() : (LerpStartTimeSeconds + DeltaSeconds);
		AdvanceCloudLerp(CurrentTimeSeconds);
	}
}

bool AMyikaSkyActor::ShouldTickIfViewportsOnly() const
{
	return true;
}

void AMyikaSkyActor::RefreshSky()
{
	ApplyConfiguredCloudMaterial();
	EnsureCloudMaterialInstance();
	ApplyCloudPresetValues(CurrentPresetValues);
	UpdateSunFromSubsystem();
}

void AMyikaSkyActor::SetPreviewTimeOfDayValue(float InPreviewTimeOfDay)
{
	PreviewTimeOfDay = FMath::Clamp(InPreviewTimeOfDay, 0.0f, 24.0f);
	UpdateSunFromSubsystem();
}

float AMyikaSkyActor::GetPreviewTimeOfDayValue() const
{
	return PreviewTimeOfDay;
}

void AMyikaSkyActor::SetAccurateModeEnabled(bool bInAccurateMode)
{
	bAccurateMode = bInAccurateMode;
	UpdateSunFromSubsystem();
}

void AMyikaSkyActor::SetStartingDateUTCValue(FDateTime InStartingDateUTC)
{
	StartingDateUTC = InStartingDateUTC;
	UpdateSunFromSubsystem();
}

void AMyikaSkyActor::SetLatitudeValue(float InLatitude)
{
	Latitude = FMath::Clamp(InLatitude, -90.0f, 90.0f);
	UpdateSunFromSubsystem();
}

void AMyikaSkyActor::SetLongitudeValue(float InLongitude)
{
	Longitude = FMath::Clamp(InLongitude, -180.0f, 180.0f);
	UpdateSunFromSubsystem();
}

void AMyikaSkyActor::SetTimeZoneOffsetHoursValue(float InTimeZoneOffsetHours)
{
	TimeZoneOffsetHours = FMath::Clamp(InTimeZoneOffsetHours, -12.0f, 14.0f);
	UpdateSunFromSubsystem();
}

const FString& AMyikaSkyActor::GetCurrentUTCDateTimeLabel() const
{
	return CurrentUTCDateTimeLabel;
}

float AMyikaSkyActor::GetCurrentSolarAltitudeDegrees() const
{
	return CurrentSolarAltitudeDegrees;
}

float AMyikaSkyActor::GetCurrentSolarAzimuthDegrees() const
{
	return CurrentSolarAzimuthDegrees;
}

void AMyikaSkyActor::LerpToPreset(UMyikaCloudPreset* TargetPreset, float DurationSeconds)
{
	if (!TargetPreset)
	{
		UE_LOG(LogMyikaSky, Warning, TEXT("LerpToPreset called with a null preset."));
		return;
	}

	TargetPresetValues.CopyFromPreset(*TargetPreset);
	StartingPresetValues = CurrentPresetValues;

	if (DurationSeconds <= KINDA_SMALL_NUMBER)
	{
		bCloudLerpActive = false;
		LerpDurationSeconds = 0.0f;
		ApplyCloudPresetValues(TargetPresetValues);
		return;
	}

	LerpStartTimeSeconds = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
	LerpDurationSeconds = DurationSeconds;
	bCloudLerpActive = true;
}

void AMyikaSkyActor::TriggerLightningNow()
{
	if (LightningComponent)
	{
		LightningComponent->TriggerStrike();
	}
}

float AMyikaSkyActor::GetLightningCooldownRemaining() const
{
	return LightningComponent ? LightningComponent->GetCooldownRemainingSeconds() : 0.0f;
}

void AMyikaSkyActor::ApplyConfiguredCloudMaterial()
{
	if (bApplyingCloudMaterial)
	{
		return;
	}
	TGuardValue<bool> ReentryGuard(bApplyingCloudMaterial, true);

	if (!VolumetricCloudComponent)
	{
		return;
	}

	UMaterialInterface* ResolvedMaterial = DefaultCloudMaterial.LoadSynchronous();
	if (ResolvedMaterial && VolumetricCloudComponent->GetMaterial() != ResolvedMaterial && !CloudMaterialInstance)
	{
		VolumetricCloudComponent->SetMaterial(ResolvedMaterial);
	}
}

void AMyikaSkyActor::ApplyCloudPresetValues(const FMyikaCloudPresetValues& Values)
{
	CurrentPresetValues = Values;

	if (!EnsureCloudMaterialInstance())
	{
		return;
	}

	CloudMaterialInstance->SetScalarParameterValue(NAME_Coverage, Values.Coverage);
	CloudMaterialInstance->SetScalarParameterValue(NAME_Morphology, Values.Morphology);
	CloudMaterialInstance->SetScalarParameterValue(NAME_WindStrength, Values.WindStrength);
	CloudMaterialInstance->SetScalarParameterValue(NAME_Extinction, Values.Extinction);
	CloudMaterialInstance->SetScalarParameterValue(NAME_Detail, Values.Detail);
	CloudMaterialInstance->SetVectorParameterValue(NAME_WindDirection, FLinearColor(Values.WindDirection.X, Values.WindDirection.Y, Values.WindDirection.Z, 0.0f));
	CloudMaterialInstance->SetVectorParameterValue(NAME_BaseColor, Values.BaseColor);
}

void AMyikaSkyActor::AdvanceCloudLerp(float CurrentTimeSeconds)
{
	const float Alpha = (LerpDurationSeconds <= KINDA_SMALL_NUMBER)
		? 1.0f
		: FMath::Clamp((CurrentTimeSeconds - LerpStartTimeSeconds) / LerpDurationSeconds, 0.0f, 1.0f);

	FMyikaCloudPresetValues BlendedValues;
	BlendedValues.Coverage = FMath::Lerp(StartingPresetValues.Coverage, TargetPresetValues.Coverage, Alpha);
	BlendedValues.Morphology = FMath::Lerp(StartingPresetValues.Morphology, TargetPresetValues.Morphology, Alpha);
	BlendedValues.WindStrength = FMath::Lerp(StartingPresetValues.WindStrength, TargetPresetValues.WindStrength, Alpha);
	BlendedValues.Extinction = FMath::Lerp(StartingPresetValues.Extinction, TargetPresetValues.Extinction, Alpha);
	BlendedValues.Detail = FMath::Lerp(StartingPresetValues.Detail, TargetPresetValues.Detail, Alpha);
	BlendedValues.WindDirection = FMath::Lerp(StartingPresetValues.WindDirection, TargetPresetValues.WindDirection, Alpha);
	BlendedValues.BaseColor = FMath::Lerp(StartingPresetValues.BaseColor, TargetPresetValues.BaseColor, Alpha);

	ApplyCloudPresetValues(BlendedValues);

	if (Alpha >= 1.0f - KINDA_SMALL_NUMBER)
	{
		bCloudLerpActive = false;
		ApplyCloudPresetValues(TargetPresetValues);
	}
}

bool AMyikaSkyActor::EnsureCloudMaterialInstance()
{
	if (CloudMaterialInstance)
	{
		return true;
	}

	if (bEnsuringCloudMaterialInstance)
	{
		return false;
	}
	TGuardValue<bool> ReentryGuard(bEnsuringCloudMaterialInstance, true);

	if (!VolumetricCloudComponent)
	{
		return false;
	}

	UMaterialInterface* SourceMaterial = VolumetricCloudComponent->GetMaterial();
	if (!SourceMaterial)
	{
		SourceMaterial = DefaultCloudMaterial.LoadSynchronous();
		if (SourceMaterial)
		{
			VolumetricCloudComponent->SetMaterial(SourceMaterial);
		}
	}

	if (!SourceMaterial)
	{
		UE_LOG(LogMyikaSky, Warning, TEXT("No cloud material is configured on %s. Cloud preset blending is disabled until MI_MyikaCloud_Default exists."), *GetName());
		return false;
	}

	CloudMaterialInstance = UMaterialInstanceDynamic::Create(SourceMaterial, this);
	if (!CloudMaterialInstance)
	{
		return false;
	}

	VolumetricCloudComponent->SetMaterial(CloudMaterialInstance);
	return true;
}

void AMyikaSkyActor::UpdateSunFromSubsystem()
{
	float TimeOfDayHours = PreviewTimeOfDay;
	UMyikaSubsystem* MyikaSubsystem = nullptr;

	if (UGameInstance* GameInstance = GetGameInstance())
	{
		MyikaSubsystem = GameInstance->GetSubsystem<UMyikaSubsystem>();
		if (MyikaSubsystem)
		{
			TimeOfDayHours = MyikaSubsystem->GetGlobalState().TimeOfDay;
		}
	}

	if (bAccurateMode)
	{
		ApplyAccurateSunRotation(TimeOfDayHours);
	}
	else
	{
		ApplyLinearSunRotation(TimeOfDayHours);
	}

	if (SunColorCurve && SunLightComponent)
	{
		SunLightComponent->SetLightColor(SunColorCurve->GetLinearColorValue(TimeOfDayHours));
	}

	if (SunIntensityCurve && SunLightComponent)
	{
		SunLightComponent->SetIntensity(FMath::Max(0.0f, SunIntensityCurve->GetFloatValue(TimeOfDayHours)));
	}

	if (SkyLuminanceCurve && SkyLightComponent)
	{
		SkyLightComponent->SetLightColor(SkyLuminanceCurve->GetLinearColorValue(TimeOfDayHours));
	}

	if (FogColorCurve && HeightFogComponent)
	{
		HeightFogComponent->SetFogInscatteringColor(FogColorCurve->GetLinearColorValue(TimeOfDayHours));
	}

	if (FogDensityCurve && HeightFogComponent)
	{
		HeightFogComponent->FogDensity = FMath::Max(0.0f, FogDensityCurve->GetFloatValue(TimeOfDayHours));
	}

	if (AutoExposureCurve && PostProcessComponent)
	{
		PostProcessComponent->Settings.AutoExposureBias = AutoExposureCurve->GetFloatValue(TimeOfDayHours);
	}

	if (SunLightComponent)
	{
		const FVector NewSunDirection = -SunLightComponent->GetForwardVector();
		if (MyikaSubsystem && !MyikaSubsystem->GetGlobalState().SunDirection.Equals(NewSunDirection, 0.001f))
		{
			MyikaSubsystem->SetSunDirection(NewSunDirection);
		}
	}
}

void AMyikaSkyActor::ApplyLinearSunRotation(float TimeOfDayHours)
{
	if (!SunLightComponent)
	{
		return;
	}

	const float WrappedTimeOfDay = FMath::Fmod(FMath::Max(TimeOfDayHours, 0.0f), 24.0f);
	const float Pitch = -90.0f + 180.0f * (WrappedTimeOfDay / 24.0f);
	const FRotator DesiredRotation(Pitch, 0.0f, 0.0f);
	SunLightComponent->SetRelativeRotation(DesiredRotation);

	CurrentSolarAltitudeDegrees = Pitch;
	CurrentSolarAzimuthDegrees = DesiredRotation.Yaw;
	CurrentUTCDateTimeLabel = TEXT("Linear time mode");
}

void AMyikaSkyActor::ApplyAccurateSunRotation(float TimeOfDayHours)
{
	// Accurate-mode follow-up will replace this with SunPosition plugin math.
	ApplyLinearSunRotation(TimeOfDayHours);

	const FDateTime CurrentDateTimeUTC = ResolveCurrentDateTimeUTC(TimeOfDayHours);
	CurrentUTCDateTimeLabel = CurrentDateTimeUTC.ToString(TEXT("%Y-%m-%d %H:%M:%S UTC"));
}

FDateTime AMyikaSkyActor::ResolveCurrentDateTimeUTC(float TimeOfDayHours) const
{
	const FDateTime BaseDate = StartingDateUTC.GetTicks() == 0
		? FDateTime(2026, 5, 3, 12, 0, 0)
		: StartingDateUTC;

	const FDateTime DayOnly(BaseDate.GetYear(), BaseDate.GetMonth(), BaseDate.GetDay(), 0, 0, 0);
	return DayOnly + FTimespan::FromHours(TimeOfDayHours);
}
