// Copyright (c) Myika AI. All rights reserved.

#include "MyikaMCPCommandRegistry.h"
#include "MyikaMCPJsonHelpers.h"
#include "MyikaMCPLog.h"
#include "MyikaSubsystem.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"

UMyikaSubsystem* FMyikaMCPCommandContext::ResolveSubsystem() const
{
	if (!GEngine)
	{
		return nullptr;
	}

	// Prefer a PIE world, then any Game world, then any world that has a GameInstance.
	UGameInstance* GameInstance = nullptr;
	for (const FWorldContext& Ctx : GEngine->GetWorldContexts())
	{
		if (Ctx.WorldType == EWorldType::PIE && Ctx.OwningGameInstance)
		{
			GameInstance = Ctx.OwningGameInstance;
			break;
		}
	}
	if (!GameInstance)
	{
		for (const FWorldContext& Ctx : GEngine->GetWorldContexts())
		{
			if (Ctx.WorldType == EWorldType::Game && Ctx.OwningGameInstance)
			{
				GameInstance = Ctx.OwningGameInstance;
				break;
			}
		}
	}
	if (!GameInstance)
	{
		for (const FWorldContext& Ctx : GEngine->GetWorldContexts())
		{
			if (Ctx.OwningGameInstance)
			{
				GameInstance = Ctx.OwningGameInstance;
				break;
			}
		}
	}
	return GameInstance ? GameInstance->GetSubsystem<UMyikaSubsystem>() : nullptr;
}

FMyikaMCPCommandRegistry& FMyikaMCPCommandRegistry::Get()
{
	static FMyikaMCPCommandRegistry Instance;
	return Instance;
}

void FMyikaMCPCommandRegistry::Register(const FString& Name, FMyikaMCPHandlerFn Handler)
{
	FScopeLock Lock(&Mutex);
	Handlers.Add(Name, MoveTemp(Handler));
	UE_LOG(LogMyikaMCP, Verbose, TEXT("Registered command: %s"), *Name);
}

void FMyikaMCPCommandRegistry::RegisterSchema(const FString& Name, const TSharedRef<FJsonObject>& Schema)
{
	FScopeLock Lock(&Mutex);
	Schemas.Add(Name, Schema);
}

void FMyikaMCPCommandRegistry::RegisterDescription(const FString& Name, const FString& Description)
{
	FScopeLock Lock(&Mutex);
	Descriptions.Add(Name, Description);
}

bool FMyikaMCPCommandRegistry::Unregister(const FString& Name)
{
	FScopeLock Lock(&Mutex);
	Schemas.Remove(Name);
	Descriptions.Remove(Name);
	return Handlers.Remove(Name) > 0;
}

bool FMyikaMCPCommandRegistry::HasCommand(const FString& Name) const
{
	FScopeLock Lock(&Mutex);
	return Handlers.Contains(Name);
}

void FMyikaMCPCommandRegistry::Clear()
{
	FScopeLock Lock(&Mutex);
	Handlers.Empty();
	Schemas.Empty();
	Descriptions.Empty();
}

TArray<FString> FMyikaMCPCommandRegistry::ListCommands() const
{
	FScopeLock Lock(&Mutex);
	TArray<FString> Names;
	Handlers.GenerateKeyArray(Names);
	Names.Sort();
	return Names;
}

TArray<FMyikaMCPCommandRegistry::FCommandInfo> FMyikaMCPCommandRegistry::ListWithMetadata() const
{
	FScopeLock Lock(&Mutex);
	TArray<FCommandInfo> Out;
	Out.Reserve(Handlers.Num());
	for (const auto& Pair : Handlers)
	{
		FCommandInfo Info;
		Info.Name = Pair.Key;
		if (const FString* DescPtr = Descriptions.Find(Pair.Key))
		{
			Info.Description = *DescPtr;
		}
		if (const TSharedPtr<FJsonObject>* SchemaPtr = Schemas.Find(Pair.Key))
		{
			Info.Schema = *SchemaPtr;
		}
		Out.Add(MoveTemp(Info));
	}
	Out.Sort([](const FCommandInfo& A, const FCommandInfo& B) { return A.Name < B.Name; });
	return Out;
}

TSharedRef<FJsonObject> FMyikaMCPCommandRegistry::Dispatch(const FString& Id, const FString& Tool, const TSharedPtr<FJsonObject>& Args) const
{
	FMyikaMCPHandlerFn Handler;
	{
		FScopeLock Lock(&Mutex);
		if (const FMyikaMCPHandlerFn* Found = Handlers.Find(Tool))
		{
			Handler = *Found;
		}
	}
	if (!Handler)
	{
		return MyikaMCP::BuildError(Id, TEXT("UNKNOWN_TOOL"), FString::Printf(TEXT("no command named '%s'"), *Tool));
	}

	FMyikaMCPCommandContext Ctx;
	Ctx.Id = Id;
	const TSharedPtr<FJsonObject> EffectiveArgs = Args.IsValid() ? Args : MakeShared<FJsonObject>();

	const FMyikaMCPHandlerResult R = Handler(Ctx, EffectiveArgs);

	if (R.bOk)
	{
		return MyikaMCP::BuildOk(Id, R.Result);
	}
	return MyikaMCP::BuildError(Id, R.ErrorCode.IsEmpty() ? TEXT("INTERNAL") : R.ErrorCode, R.ErrorMessage);
}
