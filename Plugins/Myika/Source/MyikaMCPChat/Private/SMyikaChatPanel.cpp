// Copyright (c) Myika AI. All rights reserved.

#include "SMyikaChatPanel.h"
#include "MyikaMCPChatLog.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Input/SMultiLineEditableTextBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SSeparator.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

void SMyikaChatPanel::Construct(const FArguments& InArgs)
{
	Backend = InArgs._Backend;

	const FString StatusText = Backend.IsValid()
		? FString::Printf(TEXT("Backend: %s"), *Backend->GetDisplayName())
		: FString(TEXT("No backend configured."));

	ChildSlot
	[
		SNew(SVerticalBox)

		// Header.
		+ SVerticalBox::Slot().AutoHeight().Padding(8, 6)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center)
			[
				SAssignNew(StatusLabel, STextBlock).Text(FText::FromString(StatusText))
			]
			+ SHorizontalBox::Slot().AutoWidth().Padding(4, 0)
			[
				SNew(SButton)
					.Text(FText::FromString(TEXT("New Chat")))
					.OnClicked(this, &SMyikaChatPanel::OnNewChatClicked)
					.ToolTipText(FText::FromString(TEXT("Wipe the current conversation and start fresh.")))
			]
		]

		+ SVerticalBox::Slot().AutoHeight()
		[
			SNew(SSeparator)
		]

		// Transcript.
		+ SVerticalBox::Slot().FillHeight(1.f).Padding(6)
		[
			SNew(SBorder)
			[
				SAssignNew(Transcript, SMultiLineEditableTextBox)
					.IsReadOnly(true)
					.AlwaysShowScrollbars(true)
			]
		]

		+ SVerticalBox::Slot().AutoHeight()
		[
			SNew(SSeparator)
		]

		// Composer.
		+ SVerticalBox::Slot().AutoHeight().Padding(6)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().FillWidth(1.f)
			[
				SAssignNew(Input, SEditableTextBox)
					.HintText(FText::FromString(TEXT("Ask Myika anything...")))
					.OnTextCommitted(this, &SMyikaChatPanel::OnInputCommitted)
			]
			+ SHorizontalBox::Slot().AutoWidth().Padding(4, 0, 0, 0)
			[
				SNew(SButton)
					.Text(FText::FromString(TEXT("Send")))
					.OnClicked(this, &SMyikaChatPanel::OnSendClicked)
			]
		]
	];

	Subscribe();
}

SMyikaChatPanel::~SMyikaChatPanel()
{
	Unsubscribe();
}

void SMyikaChatPanel::Subscribe()
{
	if (!Backend.IsValid())
	{
		return;
	}
	TextHandle = Backend->OnAssistantText.AddSP(this, &SMyikaChatPanel::HandleAssistantText);
	ToolHandle = Backend->OnAssistantTool.AddSP(this, &SMyikaChatPanel::HandleAssistantTool);
	DoneHandle = Backend->OnTurnComplete.AddSP(this, &SMyikaChatPanel::HandleTurnComplete);
	ErrorHandle = Backend->OnError.AddSP(this, &SMyikaChatPanel::HandleError);
}

void SMyikaChatPanel::Unsubscribe()
{
	if (!Backend.IsValid())
	{
		return;
	}
	Backend->OnAssistantText.Remove(TextHandle);
	Backend->OnAssistantTool.Remove(ToolHandle);
	Backend->OnTurnComplete.Remove(DoneHandle);
	Backend->OnError.Remove(ErrorHandle);
}

void SMyikaChatPanel::StartNewChat()
{
	if (Backend.IsValid())
	{
		Backend->NewChat();
	}
	if (Transcript.IsValid())
	{
		Transcript->SetText(FText::GetEmpty());
	}
	bAssistantTurnInProgress = false;
}

void SMyikaChatPanel::Append(const FString& Text)
{
	if (!Transcript.IsValid())
	{
		return;
	}
	const FString Existing = Transcript->GetText().ToString();
	Transcript->SetText(FText::FromString(Existing + Text));
}

void SMyikaChatPanel::AppendUserBlock(const FString& Text)
{
	Append(FString::Printf(TEXT("\n\nYou: %s\n"), *Text));
}

void SMyikaChatPanel::AppendAssistantHeader()
{
	Append(TEXT("\nMyika: "));
}

void SMyikaChatPanel::AppendToolCallLine(const FMyikaChatToolCall& Call)
{
	FString Args;
	if (Call.Input.IsValid())
	{
		TSharedRef<TJsonWriter<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>> Writer =
			TJsonWriterFactory<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>::Create(&Args);
		FJsonSerializer::Serialize(Call.Input.ToSharedRef(), Writer);
	}
	Append(FString::Printf(TEXT("\n  → %s(%s)\n"), *Call.Name, *Args));
}

void SMyikaChatPanel::AppendErrorLine(const FString& Message)
{
	Append(FString::Printf(TEXT("\n[error] %s\n"), *Message));
}

FReply SMyikaChatPanel::OnSendClicked()
{
	if (!Input.IsValid() || !Backend.IsValid())
	{
		return FReply::Handled();
	}
	const FString Text = Input->GetText().ToString().TrimStartAndEnd();
	if (Text.IsEmpty())
	{
		return FReply::Handled();
	}
	if (Backend->IsBusy())
	{
		AppendErrorLine(TEXT("Backend is busy — wait for the current response."));
		return FReply::Handled();
	}

	AppendUserBlock(Text);
	AppendAssistantHeader();
	bAssistantTurnInProgress = true;
	Input->SetText(FText::GetEmpty());
	Backend->SendUserMessage(Text);
	return FReply::Handled();
}

FReply SMyikaChatPanel::OnNewChatClicked()
{
	StartNewChat();
	return FReply::Handled();
}

FReply SMyikaChatPanel::OnInputKeyDown(const FGeometry& /*Geom*/, const FKeyEvent& KeyEvent)
{
	// Single-line SEditableTextBox commits on Enter via OnTextCommitted; no extra
	// handling needed. Stub kept for potential future Shift+Enter / multi-line.
	if (KeyEvent.GetKey() == EKeys::Enter && !KeyEvent.IsShiftDown())
	{
		return FReply::Unhandled();
	}
	return FReply::Unhandled();
}

void SMyikaChatPanel::OnInputCommitted(const FText& Text, ETextCommit::Type CommitType)
{
	if (CommitType == ETextCommit::OnEnter)
	{
		OnSendClicked();
	}
}

void SMyikaChatPanel::HandleAssistantText(const FString& Delta)
{
	if (!bAssistantTurnInProgress)
	{
		AppendAssistantHeader();
		bAssistantTurnInProgress = true;
	}
	Append(Delta);
}

void SMyikaChatPanel::HandleAssistantTool(const FMyikaChatToolCall& Call)
{
	AppendToolCallLine(Call);
}

void SMyikaChatPanel::HandleTurnComplete(const FString& /*FullText*/, const FString& StopReason)
{
	bAssistantTurnInProgress = false;
	if (!StopReason.IsEmpty() && StopReason != TEXT("end_turn"))
	{
		Append(FString::Printf(TEXT("\n[stop: %s]\n"), *StopReason));
	}
	else
	{
		Append(TEXT("\n"));
	}
}

void SMyikaChatPanel::HandleError(const FString& Message)
{
	bAssistantTurnInProgress = false;
	AppendErrorLine(Message);
}
