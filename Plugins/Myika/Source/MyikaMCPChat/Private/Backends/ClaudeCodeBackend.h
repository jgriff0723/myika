// Copyright (c) Myika AI. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "MyikaChatBackend.h"
#include "HAL/Runnable.h"
#include "HAL/ThreadSafeBool.h"

class FRunnableThread;

/**
 * Subprocess-based backend that drives the local `claude` CLI. Each user
 * message spawns a fresh `claude --print --output-format stream-json` process,
 * pipes the rebuilt history into stdin, parses NDJSON from stdout line-by-line,
 * and fans out events to delegates on the game thread.
 *
 * Multi-turn is reconstructed by stitching prior turns into a single prompt
 * (the CLI is stateless in --print mode), mirroring the proven pattern from
 * the earlier Tauri claude.rs implementation.
 */
class FClaudeCodeBackend : public IMyikaChatBackend, public TSharedFromThis<FClaudeCodeBackend>, public FRunnable
{
public:
	FClaudeCodeBackend();
	virtual ~FClaudeCodeBackend() override;

	// IMyikaChatBackend
	virtual void SendUserMessage(const FString& Text) override;
	virtual bool IsBusy() const override { return bBusy; }
	virtual void NewChat() override;
	virtual FString GetDisplayName() const override;

	// FRunnable
	virtual bool Init() override { return true; }
	virtual uint32 Run() override;
	virtual void Stop() override;
	virtual void Exit() override;

private:
	void DispatchOnGameThread(TFunction<void()> Fn);
	void ParseStreamLine(const FString& Line, FString& InOutAccumulatedText, int32& InOutEmittedLen);
	FString BuildPromptFromHistory(const FString& NewUserMessage) const;
	FString ResolveExePath() const;

	// Conversation history (just role+plain text — Claude CLI in --print
	// stream-json mode does not preserve assistant tool_use blocks across calls,
	// and we don't dispatch tools ourselves on this path).
	TArray<TPair<FString, FString>> History;

	FRunnableThread* Thread = nullptr;
	FString PendingMessage;
	FThreadSafeBool bStopRequested = false;
	FThreadSafeBool bBusy = false;
};
