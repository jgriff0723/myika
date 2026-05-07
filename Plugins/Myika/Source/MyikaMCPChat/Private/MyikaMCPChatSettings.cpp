// Copyright (c) Myika AI. All rights reserved.

#include "MyikaMCPChatSettings.h"
#include "MyikaMCPChatLog.h"
#include "Interfaces/IPluginManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

UMyikaMCPChatSettings::UMyikaMCPChatSettings()
{
	SectionName = TEXT("Myika MCP Chat");
}

FString UMyikaMCPChatSettings::GetEffectiveAnthropicApiKey()
{
	const UMyikaMCPChatSettings* S = GetDefault<UMyikaMCPChatSettings>();
	if (S && !S->AnthropicApiKey.IsEmpty())
	{
		return S->AnthropicApiKey;
	}
	return FPlatformMisc::GetEnvironmentVariable(TEXT("ANTHROPIC_API_KEY"));
}

FString UMyikaMCPChatSettings::GetEffectiveSystemPrompt()
{
	const UMyikaMCPChatSettings* S = GetDefault<UMyikaMCPChatSettings>();
	if (S && !S->SystemPrompt.IsEmpty())
	{
		return S->SystemPrompt;
	}

	const TSharedPtr<IPlugin> Plugin = IPluginManager::Get().FindPlugin(TEXT("Myika"));
	if (!Plugin.IsValid())
	{
		UE_LOG(LogMyikaMCPChat, Warning, TEXT("Myika plugin not found; using empty system prompt."));
		return FString();
	}

	const FString Path = FPaths::Combine(Plugin->GetBaseDir(), TEXT("Resources"), TEXT("MyikaSystemPrompt.txt"));
	FString Out;
	if (!FFileHelper::LoadFileToString(Out, *Path))
	{
		UE_LOG(LogMyikaMCPChat, Warning, TEXT("Could not load %s; using empty system prompt."), *Path);
		return FString();
	}
	return Out;
}
