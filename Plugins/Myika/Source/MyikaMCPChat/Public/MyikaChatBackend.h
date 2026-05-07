// Copyright (c) Myika AI. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "MyikaChatTypes.h"
#include "Delegates/DelegateCombinations.h"

/**
 * Streaming-text delta from the assistant. Multiple firings per turn — append
 * to the running message in the UI.
 */
DECLARE_MULTICAST_DELEGATE_OneParam(FOnAssistantText, const FString& /*TextDelta*/);

/**
 * The assistant invoked a tool. The backend may already have dispatched it
 * (AnthropicDirect) or may be waiting on an external dispatcher (ClaudeCode
 * via MCP). Either way, the UI should show the call to the user.
 */
DECLARE_MULTICAST_DELEGATE_OneParam(FOnAssistantTool, const FMyikaChatToolCall& /*Call*/);

/**
 * Assistant turn finished — full assembled assistant text + the final
 * stop reason (e.g. "end_turn", "max_tokens", "tool_use" if we hit the
 * iteration cap mid-loop).
 */
DECLARE_MULTICAST_DELEGATE_TwoParams(FOnTurnComplete, const FString& /*FullText*/, const FString& /*StopReason*/);

/**
 * Backend hit an error. Backend may decide to keep itself usable for the next
 * SendUserMessage; the UI should report the error and re-enable input.
 */
DECLARE_MULTICAST_DELEGATE_OneParam(FOnError, const FString& /*Message*/);

/**
 * Abstract chat backend. Two MVP implementations:
 *   - FClaudeCodeBackend     spawns the local `claude` CLI (subprocess + stream-json)
 *   - FAnthropicDirectBackend  posts to api.anthropic.com and runs the tool-use loop in-process
 *
 * Construction is up to the module — the chat panel just sees the interface.
 */
class MYIKAMCPCHAT_API IMyikaChatBackend
{
public:
	virtual ~IMyikaChatBackend() = default;

	/** Send a user message. Fires the delegates as the turn unfolds. */
	virtual void SendUserMessage(const FString& Text) = 0;

	/** True while a SendUserMessage is in flight. SendUserMessage is rejected (with FOnError) while busy. */
	virtual bool IsBusy() const = 0;

	/** Forget all conversation history. Fires nothing. */
	virtual void NewChat() = 0;

	/** A short human-readable label (e.g. "Claude Code (sonnet)"). For the panel header. */
	virtual FString GetDisplayName() const = 0;

	// Delegates.
	FOnAssistantText OnAssistantText;
	FOnAssistantTool OnAssistantTool;
	FOnTurnComplete OnTurnComplete;
	FOnError OnError;
};
