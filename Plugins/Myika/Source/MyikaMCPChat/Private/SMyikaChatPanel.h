// Copyright (c) Myika AI. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "MyikaChatBackend.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/SCompoundWidget.h"

class SMultiLineEditableTextBox;
class SEditableTextBox;
class STextBlock;

/**
 * The dockable chat panel. Subscribes to the four IMyikaChatBackend delegates
 * and updates a scrolling transcript + a single-line input box.
 *
 * Plaintext only for MVP — markdown rendering is Phase 2.
 */
class SMyikaChatPanel : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SMyikaChatPanel) {}
		SLATE_ARGUMENT(TSharedPtr<IMyikaChatBackend>, Backend)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	virtual ~SMyikaChatPanel() override;

	void StartNewChat();

private:
	void Subscribe();
	void Unsubscribe();
	void Append(const FString& Text);
	void AppendUserBlock(const FString& Text);
	void AppendAssistantHeader();
	void AppendToolCallLine(const FMyikaChatToolCall& Call);
	void AppendErrorLine(const FString& Message);

	FReply OnSendClicked();
	FReply OnNewChatClicked();
	FReply OnInputKeyDown(const FGeometry& Geom, const FKeyEvent& KeyEvent);
	void OnInputCommitted(const FText& Text, ETextCommit::Type CommitType);

	void HandleAssistantText(const FString& Delta);
	void HandleAssistantTool(const FMyikaChatToolCall& Call);
	void HandleTurnComplete(const FString& FullText, const FString& StopReason);
	void HandleError(const FString& Message);

	TSharedPtr<IMyikaChatBackend> Backend;
	TSharedPtr<SMultiLineEditableTextBox> Transcript;
	TSharedPtr<SEditableTextBox> Input;
	TSharedPtr<STextBlock> StatusLabel;

	bool bAssistantTurnInProgress = false;

	FDelegateHandle TextHandle;
	FDelegateHandle ToolHandle;
	FDelegateHandle DoneHandle;
	FDelegateHandle ErrorHandle;
};
