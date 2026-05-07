// Copyright (c) Myika AI. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Dom/JsonObject.h"
#include "MyikaChatTypes.generated.h"

UENUM(BlueprintType)
enum class EMyikaChatBackendKind : uint8
{
	/** Spawn the local `claude` CLI as a subprocess and stream NDJSON. Free-tier friendly; uses the user's Claude Code login. */
	ClaudeCode  UMETA(DisplayName = "Claude Code (local CLI)"),

	/** Direct HTTP to api.anthropic.com using ANTHROPIC_API_KEY. Lower latency, requires a paid API key. */
	AnthropicDirect  UMETA(DisplayName = "Anthropic API (direct)")
};

/** One assistant tool call, fanned out via FOnAssistantTool before its result is fed back to the model. */
struct MYIKAMCPCHAT_API FMyikaChatToolCall
{
	FString Id;                      // Anthropic tool_use id (also reused as our request id)
	FString Name;                    // e.g. "set_time_of_day"
	TSharedPtr<FJsonObject> Input;   // tool input args object
};

/** A normalized chat message — backend-agnostic content blocks. */
struct MYIKAMCPCHAT_API FMyikaChatContentBlock
{
	enum class EKind
	{
		Text,
		ToolUse,
		ToolResult
	};

	EKind Kind = EKind::Text;
	FString Text;                    // Text or ToolResultText (when ToolResult)

	// ToolUse:
	FString ToolUseId;
	FString ToolName;
	TSharedPtr<FJsonObject> ToolInput;

	// ToolResult:
	bool bToolResultIsError = false;
};

struct MYIKAMCPCHAT_API FMyikaChatMessage
{
	FString Role;                    // "user" | "assistant"
	TArray<FMyikaChatContentBlock> Content;

	static FMyikaChatMessage UserText(const FString& Text);
};
