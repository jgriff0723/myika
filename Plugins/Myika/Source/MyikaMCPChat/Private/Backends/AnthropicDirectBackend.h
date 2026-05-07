// Copyright (c) Myika AI. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "MyikaChatBackend.h"

class IHttpRequest;

class FAnthropicDirectBackend : public IMyikaChatBackend, public TSharedFromThis<FAnthropicDirectBackend>
{
public:
	FAnthropicDirectBackend();
	virtual ~FAnthropicDirectBackend() override;

	// IMyikaChatBackend
	virtual void SendUserMessage(const FString& Text) override;
	virtual bool IsBusy() const override { return bBusy; }
	virtual void NewChat() override;
	virtual FString GetDisplayName() const override;

private:
	void StartTurn();
	void IssueRequest();
	void OnHttpComplete(const FString& Body, int32 StatusCode, bool bConnectedSuccessfully);
	void HandleToolUseBlocks(const TArray<FMyikaChatContentBlock>& AssistantBlocks);
	void Fail(const FString& Message);
	void Finish(const FString& StopReason, const FString& AssistantText);

	TArray<FMyikaChatMessage> Messages;
	TArray<TSharedRef<class FJsonObject>> ToolDefs; // rebuilt each turn from registry
	TSharedPtr<IHttpRequest, ESPMode::ThreadSafe> InflightRequest;
	int32 IterationsThisTurn = 0;
	bool bBusy = false;
	FString AccumulatedAssistantText;
};
