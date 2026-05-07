// Copyright (c) Myika AI. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Dom/JsonObject.h"
#include "Templates/Function.h"

class UMyikaSubsystem;

/** Context handed to a command handler. */
struct FMyikaMCPCommandContext
{
	/** Request id for correlating responses. */
	FString Id;

	/**
	 * Resolve the Myika subsystem from the active game/PIE world.
	 * Must be called on the game thread. Returns nullptr if no GameInstance is available.
	 */
	UMyikaSubsystem* ResolveSubsystem() const;
};

/**
 * A handler returns the response result FJsonObject (just the inner result),
 * or throws by setting OutError code/message and returning a null shared ptr.
 *
 * Args is the inner "args" object from the request (never null — empty object passed if missing).
 */
struct FMyikaMCPHandlerResult
{
	bool bOk = true;
	TSharedPtr<FJsonObject> Result;          // valid when bOk == true
	FString ErrorCode;                       // valid when bOk == false
	FString ErrorMessage;
};

using FMyikaMCPHandlerFn = TFunction<FMyikaMCPHandlerResult(const FMyikaMCPCommandContext&, const TSharedPtr<FJsonObject>& Args)>;

class MYIKAMCP_API FMyikaMCPCommandRegistry
{
public:
	static FMyikaMCPCommandRegistry& Get();

	void Register(const FString& Name, FMyikaMCPHandlerFn Handler);

	/**
	 * Attach a JSON Schema input description for a registered tool. Optional —
	 * only consumed by in-process AI clients (e.g. MyikaMCPChat) that need to
	 * forward tool definitions to an LLM provider. The TCP / Python-MCP path
	 * has its own schemas defined in FastMCP.
	 */
	void RegisterSchema(const FString& Name, const TSharedRef<FJsonObject>& Schema);
	void RegisterDescription(const FString& Name, const FString& Description);

	bool Unregister(const FString& Name);
	bool HasCommand(const FString& Name) const;
	void Clear();

	/** Dispatch synchronously. Returns the full response envelope (with id, ok, result/error). */
	TSharedRef<FJsonObject> Dispatch(const FString& Id, const FString& Tool, const TSharedPtr<FJsonObject>& Args) const;

	/** List of registered command names — used by tooling/diagnostics. */
	TArray<FString> ListCommands() const;

	struct FCommandInfo
	{
		FString Name;
		FString Description;
		TSharedPtr<FJsonObject> Schema;
	};

	/** Snapshot of every registered command + its schema/description. Safe to call from any thread. */
	TArray<FCommandInfo> ListWithMetadata() const;

private:
	mutable FCriticalSection Mutex;
	TMap<FString, FMyikaMCPHandlerFn> Handlers;
	TMap<FString, TSharedPtr<FJsonObject>> Schemas;
	TMap<FString, FString> Descriptions;
};
