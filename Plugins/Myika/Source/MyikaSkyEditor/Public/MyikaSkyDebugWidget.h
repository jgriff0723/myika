// Copyright (c) Myika AI. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "EditorUtilityWidget.h"
#include "MyikaSkyDebugWidget.generated.h"

class UButton;
class USlider;
class UTextBlock;
class AMyikaSkyActor;
class UMyikaSubsystem;
class UWorld;

UCLASS(DisplayName = "Myika Sky Debug Widget")
class MYIKASKYEDITOR_API UMyikaSkyDebugWidget : public UEditorUtilityWidget
{
	GENERATED_BODY()

public:
	UMyikaSkyDebugWidget(const FObjectInitializer& ObjectInitializer);

	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
	/** Time-of-day slider mirrored to `UMyikaSubsystem::SetTimeOfDay`. */
	UPROPERTY(Transient)
	TObjectPtr<USlider> TimeOfDaySlider;

	/** Label showing the current hour value from the subsystem. */
	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> TimeOfDayValueLabel;

	/** Label showing the live `EMyikaPartOfDay` display name. */
	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> PartOfDayLabel;

	/** Storm-intensity slider mirrored to `UMyikaSubsystem::SetStormIntensity`. */
	UPROPERTY(Transient)
	TObjectPtr<USlider> StormIntensitySlider;

	/** Label showing the current storm intensity from the subsystem. */
	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> StormIntensityValueLabel;

	/** Label showing the lightning cooldown remaining from the sky actor. */
	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> LightningCooldownLabel;

	/** Reset button that snaps the subsystem back to noon. */
	UPROPERTY(Transient)
	TObjectPtr<UButton> ResetButton;

	/** Button that triggers a lightning strike immediately. */
	UPROPERTY(Transient)
	TObjectPtr<UButton> TriggerLightningButton;

	/** Guard against recursive slider updates while syncing from the subsystem. */
	bool bSyncingControls = false;

	void BuildWidgetTreeIfNeeded();
	void SyncFromSubsystem(bool bUpdateSlider);
	UWorld* ResolveTargetWorld() const;
	AMyikaSkyActor* ResolveSkyActor() const;
	UMyikaSubsystem* ResolveSubsystem() const;

	UFUNCTION()
	void HandleTimeOfDayChanged(float NewValue);

	UFUNCTION()
	void HandleStormIntensityChanged(float NewValue);

	UFUNCTION()
	void HandleTriggerLightningClicked();

	UFUNCTION()
	void HandleResetClicked();
};
