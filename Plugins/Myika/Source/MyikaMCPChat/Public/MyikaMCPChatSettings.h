// Copyright (c) Myika AI. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "MyikaChatTypes.h"
#include "MyikaMCPChatSettings.generated.h"

UCLASS(config = Editor, defaultconfig, meta = (DisplayName = "Myika MCP Chat"))
class MYIKAMCPCHAT_API UMyikaMCPChatSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UMyikaMCPChatSettings();

	virtual FName GetCategoryName() const override { return TEXT("Plugins"); }

	/** Which backend the chat panel uses. */
	UPROPERTY(EditAnywhere, config, Category = "Myika MCP Chat")
	EMyikaChatBackendKind Backend = EMyikaChatBackendKind::ClaudeCode;

	/** Anthropic model used by the AnthropicDirect backend. */
	UPROPERTY(EditAnywhere, config, Category = "Myika MCP Chat|Anthropic Direct")
	FString AnthropicModel = TEXT("claude-sonnet-4-6");

	/**
	 * Anthropic API key. If empty, falls back to the ANTHROPIC_API_KEY environment
	 * variable. Stored in DefaultEditor.ini if you set it here, so prefer the env
	 * var when source-control hygiene matters.
	 */
	UPROPERTY(EditAnywhere, config, Category = "Myika MCP Chat|Anthropic Direct", meta = (PasswordField = true))
	FString AnthropicApiKey;

	/** API endpoint. Override only for proxies / staging. */
	UPROPERTY(EditAnywhere, config, AdvancedDisplay, Category = "Myika MCP Chat|Anthropic Direct")
	FString AnthropicEndpoint = TEXT("https://api.anthropic.com/v1/messages");

	/** Anthropic API version header. */
	UPROPERTY(EditAnywhere, config, AdvancedDisplay, Category = "Myika MCP Chat|Anthropic Direct")
	FString AnthropicVersion = TEXT("2023-06-01");

	/** Maximum response tokens per turn (Anthropic backend). */
	UPROPERTY(EditAnywhere, config, Category = "Myika MCP Chat|Anthropic Direct", meta = (ClampMin = 256, ClampMax = 32768))
	int32 AnthropicMaxTokens = 4096;

	/** Path to the `claude` CLI binary used by the ClaudeCode backend. Empty = expect "claude" on PATH. */
	UPROPERTY(EditAnywhere, config, Category = "Myika MCP Chat|Claude Code")
	FString ClaudeCodeExePath;

	/** Hard cap on tool-use iterations per user prompt. Prevents runaway loops. */
	UPROPERTY(EditAnywhere, config, Category = "Myika MCP Chat", meta = (ClampMin = 1, ClampMax = 50))
	int32 MaxToolIterations = 10;

	/**
	 * System prompt sent on every turn. Empty = load Resources/MyikaSystemPrompt.txt
	 * shipped with the plugin. Multi-line.
	 */
	UPROPERTY(EditAnywhere, config, Category = "Myika MCP Chat", meta = (MultiLine = true))
	FString SystemPrompt;

	/** Resolve the effective Anthropic API key (settings field, then ANTHROPIC_API_KEY env). */
	static FString GetEffectiveAnthropicApiKey();

	/** Resolve the system prompt — settings override or shipped Resources/MyikaSystemPrompt.txt. */
	static FString GetEffectiveSystemPrompt();
};
