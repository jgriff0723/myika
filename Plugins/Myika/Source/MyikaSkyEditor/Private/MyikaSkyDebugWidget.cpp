// Copyright (c) Myika AI. All rights reserved.

#include "MyikaSkyDebugWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/Slider.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Editor.h"
#include "Engine/GameInstance.h"
#include "EngineUtils.h"
#include "MyikaSkyActor.h"
#include "MyikaSubsystem.h"
#include "MyikaTypes.h"

UMyikaSkyDebugWidget::UMyikaSkyDebugWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	TabDisplayName = FText::FromString(TEXT("Myika Sky Debug"));
}

TSharedRef<SWidget> UMyikaSkyDebugWidget::RebuildWidget()
{
	BuildWidgetTreeIfNeeded();
	return Super::RebuildWidget();
}

void UMyikaSkyDebugWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (TimeOfDaySlider)
	{
		TimeOfDaySlider->OnValueChanged.RemoveDynamic(this, &UMyikaSkyDebugWidget::HandleTimeOfDayChanged);
		TimeOfDaySlider->OnValueChanged.AddDynamic(this, &UMyikaSkyDebugWidget::HandleTimeOfDayChanged);
	}

	if (ResetButton)
	{
		ResetButton->OnClicked.RemoveDynamic(this, &UMyikaSkyDebugWidget::HandleResetClicked);
		ResetButton->OnClicked.AddDynamic(this, &UMyikaSkyDebugWidget::HandleResetClicked);
	}

	if (StormIntensitySlider)
	{
		StormIntensitySlider->OnValueChanged.RemoveDynamic(this, &UMyikaSkyDebugWidget::HandleStormIntensityChanged);
		StormIntensitySlider->OnValueChanged.AddDynamic(this, &UMyikaSkyDebugWidget::HandleStormIntensityChanged);
	}

	if (TriggerLightningButton)
	{
		TriggerLightningButton->OnClicked.RemoveDynamic(this, &UMyikaSkyDebugWidget::HandleTriggerLightningClicked);
		TriggerLightningButton->OnClicked.AddDynamic(this, &UMyikaSkyDebugWidget::HandleTriggerLightningClicked);
	}

	SyncFromSubsystem(true);
}

void UMyikaSkyDebugWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	SyncFromSubsystem(false);
}

void UMyikaSkyDebugWidget::BuildWidgetTreeIfNeeded()
{
	if (!WidgetTree || WidgetTree->FindWidget(TEXT("SkyDebugRoot")))
	{
		return;
	}

	UVerticalBox* RootBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("SkyDebugRoot"));
	WidgetTree->RootWidget = RootBox;

	UTextBlock* TitleLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("TitleLabel"));
	TitleLabel->SetText(FText::FromString(TEXT("Myika Sky Debug")));
	if (UVerticalBoxSlot* VerticalSlot = RootBox->AddChildToVerticalBox(TitleLabel))
	{
		VerticalSlot->SetPadding(FMargin(8.0f, 8.0f, 8.0f, 4.0f));
	}

	TimeOfDayValueLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("TimeOfDayValueLabel"));
	TimeOfDayValueLabel->SetText(FText::FromString(TEXT("Time Of Day: 12.00 h")));
	if (UVerticalBoxSlot* VerticalSlot = RootBox->AddChildToVerticalBox(TimeOfDayValueLabel))
	{
		VerticalSlot->SetPadding(FMargin(8.0f, 4.0f));
	}

	TimeOfDaySlider = WidgetTree->ConstructWidget<USlider>(USlider::StaticClass(), TEXT("TimeOfDaySlider"));
	TimeOfDaySlider->SetMinValue(0.0f);
	TimeOfDaySlider->SetMaxValue(24.0f);
	TimeOfDaySlider->SetStepSize(0.1f);
	TimeOfDaySlider->SetValue(12.0f);
	if (UVerticalBoxSlot* VerticalSlot = RootBox->AddChildToVerticalBox(TimeOfDaySlider))
	{
		VerticalSlot->SetPadding(FMargin(8.0f, 4.0f));
	}

	PartOfDayLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("PartOfDayLabel"));
	PartOfDayLabel->SetText(FText::FromString(TEXT("Part Of Day: Noon")));
	if (UVerticalBoxSlot* VerticalSlot = RootBox->AddChildToVerticalBox(PartOfDayLabel))
	{
		VerticalSlot->SetPadding(FMargin(8.0f, 4.0f));
	}

	StormIntensityValueLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("StormIntensityValueLabel"));
	StormIntensityValueLabel->SetText(FText::FromString(TEXT("Storm Intensity: 0.00")));
	if (UVerticalBoxSlot* VerticalSlot = RootBox->AddChildToVerticalBox(StormIntensityValueLabel))
	{
		VerticalSlot->SetPadding(FMargin(8.0f, 4.0f));
	}

	StormIntensitySlider = WidgetTree->ConstructWidget<USlider>(USlider::StaticClass(), TEXT("StormIntensitySlider"));
	StormIntensitySlider->SetMinValue(0.0f);
	StormIntensitySlider->SetMaxValue(1.0f);
	StormIntensitySlider->SetStepSize(0.01f);
	StormIntensitySlider->SetValue(0.0f);
	if (UVerticalBoxSlot* VerticalSlot = RootBox->AddChildToVerticalBox(StormIntensitySlider))
	{
		VerticalSlot->SetPadding(FMargin(8.0f, 4.0f));
	}

	LightningCooldownLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("LightningCooldownLabel"));
	LightningCooldownLabel->SetText(FText::FromString(TEXT("Lightning Cooldown: n/a")));
	if (UVerticalBoxSlot* VerticalSlot = RootBox->AddChildToVerticalBox(LightningCooldownLabel))
	{
		VerticalSlot->SetPadding(FMargin(8.0f, 4.0f));
	}

	TriggerLightningButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("TriggerLightningButton"));
	UTextBlock* TriggerLightningText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("TriggerLightningText"));
	TriggerLightningText->SetText(FText::FromString(TEXT("Trigger lightning now")));
	TriggerLightningButton->AddChild(TriggerLightningText);
	if (UVerticalBoxSlot* VerticalSlot = RootBox->AddChildToVerticalBox(TriggerLightningButton))
	{
		VerticalSlot->SetPadding(FMargin(8.0f, 8.0f, 8.0f, 4.0f));
	}

	ResetButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("ResetButton"));
	UTextBlock* ResetText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("ResetText"));
	ResetText->SetText(FText::FromString(TEXT("Reset")));
	ResetButton->AddChild(ResetText);
	if (UVerticalBoxSlot* VerticalSlot = RootBox->AddChildToVerticalBox(ResetButton))
	{
		VerticalSlot->SetPadding(FMargin(8.0f, 8.0f));
	}
}

void UMyikaSkyDebugWidget::SyncFromSubsystem(bool bUpdateSlider)
{
	UMyikaSubsystem* Subsystem = ResolveSubsystem();
	if (!Subsystem)
	{
		if (TimeOfDayValueLabel)
		{
			TimeOfDayValueLabel->SetText(FText::FromString(TEXT("Time Of Day: PIE inactive")));
		}

		if (PartOfDayLabel)
		{
			PartOfDayLabel->SetText(FText::FromString(TEXT("Part Of Day: start PIE on L_SkyTest")));
		}

		if (StormIntensityValueLabel)
		{
			StormIntensityValueLabel->SetText(FText::FromString(TEXT("Storm Intensity: PIE inactive")));
		}

		if (LightningCooldownLabel)
		{
			LightningCooldownLabel->SetText(FText::FromString(TEXT("Lightning Cooldown: start PIE on L_SkyTest")));
		}

		return;
	}

	const FMyikaGlobalState& State = Subsystem->GetGlobalState();
	if (TimeOfDayValueLabel)
	{
		TimeOfDayValueLabel->SetText(FText::FromString(FString::Printf(TEXT("Time Of Day: %.2f h"), State.TimeOfDay)));
	}

	if (PartOfDayLabel)
	{
		const UEnum* PartOfDayEnum = StaticEnum<EMyikaPartOfDay>();
		const FText PartOfDayText = PartOfDayEnum
			? PartOfDayEnum->GetDisplayNameTextByValue(static_cast<int64>(State.PartOfDay))
			: FText::FromString(TEXT("Unknown"));
		PartOfDayLabel->SetText(FText::FromString(FString::Printf(TEXT("Part Of Day: %s"), *PartOfDayText.ToString())));
	}

	if (StormIntensityValueLabel)
	{
		StormIntensityValueLabel->SetText(FText::FromString(FString::Printf(TEXT("Storm Intensity: %.2f"), State.StormIntensity)));
	}

	if (LightningCooldownLabel)
	{
		if (AMyikaSkyActor* SkyActor = ResolveSkyActor())
		{
			LightningCooldownLabel->SetText(
				FText::FromString(FString::Printf(TEXT("Lightning Cooldown: %.2f s"), SkyActor->GetLightningCooldownRemaining())));
		}
		else
		{
			LightningCooldownLabel->SetText(FText::FromString(TEXT("Lightning Cooldown: no MyikaSky actor in world")));
		}
	}

	if (bUpdateSlider && TimeOfDaySlider && !bSyncingControls && !FMath::IsNearlyEqual(TimeOfDaySlider->GetValue(), State.TimeOfDay))
	{
		TGuardValue<bool> SyncGuard(bSyncingControls, true);
		TimeOfDaySlider->SetValue(State.TimeOfDay);
	}

	if (bUpdateSlider && StormIntensitySlider && !bSyncingControls && !FMath::IsNearlyEqual(StormIntensitySlider->GetValue(), State.StormIntensity))
	{
		TGuardValue<bool> SyncGuard(bSyncingControls, true);
		StormIntensitySlider->SetValue(State.StormIntensity);
	}
}

UWorld* UMyikaSkyDebugWidget::ResolveTargetWorld() const
{
	if (GEditor)
	{
		if (UWorld* PlayWorld = GEditor->PlayWorld)
		{
			return PlayWorld;
		}

		return GEditor->GetEditorWorldContext().World();
	}

	return nullptr;
}

AMyikaSkyActor* UMyikaSkyDebugWidget::ResolveSkyActor() const
{
	if (UWorld* World = ResolveTargetWorld())
	{
		for (TActorIterator<AMyikaSkyActor> It(World); It; ++It)
		{
			return *It;
		}
	}

	return nullptr;
}

UMyikaSubsystem* UMyikaSkyDebugWidget::ResolveSubsystem() const
{
	if (UWorld* World = ResolveTargetWorld())
	{
		if (UGameInstance* GameInstance = World->GetGameInstance())
		{
			return GameInstance->GetSubsystem<UMyikaSubsystem>();
		}
	}

	return nullptr;
}

void UMyikaSkyDebugWidget::HandleTimeOfDayChanged(float NewValue)
{
	if (bSyncingControls)
	{
		return;
	}

	if (UMyikaSubsystem* Subsystem = ResolveSubsystem())
	{
		Subsystem->SetTimeOfDay(NewValue);
		SyncFromSubsystem(false);
	}
}

void UMyikaSkyDebugWidget::HandleStormIntensityChanged(float NewValue)
{
	if (bSyncingControls)
	{
		return;
	}

	if (UMyikaSubsystem* Subsystem = ResolveSubsystem())
	{
		Subsystem->SetStormIntensity(NewValue);
		SyncFromSubsystem(false);
	}
}

void UMyikaSkyDebugWidget::HandleTriggerLightningClicked()
{
	if (AMyikaSkyActor* SkyActor = ResolveSkyActor())
	{
		SkyActor->TriggerLightningNow();
		SyncFromSubsystem(false);
	}
}

void UMyikaSkyDebugWidget::HandleResetClicked()
{
	if (UMyikaSubsystem* Subsystem = ResolveSubsystem())
	{
		Subsystem->SetTimeOfDay(12.0f);
		Subsystem->SetStormIntensity(0.0f);
		SyncFromSubsystem(true);
	}
}
