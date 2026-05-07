// Copyright (c) Myika AI. All rights reserved.

#include "MyikaMCPCommands.h"
#include "MyikaMCPGameThread.h"
#include "MyikaMCPJsonHelpers.h"
#include "MyikaSubsystem.h"
#include "Dom/JsonObject.h"

namespace MyikaMCP::Commands
{
	FMyikaMCPHandlerFn MakeGetGlobalStateHandler()
	{
		return [](const FMyikaMCPCommandContext& Ctx, const TSharedPtr<FJsonObject>&) -> FMyikaMCPHandlerResult
		{
			FMyikaMCPHandlerResult R;

			TSharedPtr<FJsonObject> StateJson;
			bool bResolved = false;
			MyikaMCP::RunOnGameThreadVoid([&]()
			{
				if (UMyikaSubsystem* Sub = Ctx.ResolveSubsystem())
				{
					bResolved = true;
					StateJson = MyikaMCP::StateToJson(Sub->GetGlobalState());
				}
			});

			if (!bResolved)
			{
				R.bOk = false;
				R.ErrorCode = TEXT("NO_SUBSYSTEM");
				R.ErrorMessage = TEXT("UMyikaSubsystem unavailable (start PIE or run game)");
				return R;
			}

			R.bOk = true;
			R.Result = MakeShared<FJsonObject>();
			R.Result->SetObjectField(TEXT("state"), StateJson);
			return R;
		};
	}

	FMyikaMCPHandlerFn MakeSetGlobalStateHandler()
	{
		return [](const FMyikaMCPCommandContext& Ctx, const TSharedPtr<FJsonObject>& Args) -> FMyikaMCPHandlerResult
		{
			FMyikaMCPHandlerResult R;

			const TSharedPtr<FJsonObject>* StateObjPtr = nullptr;
			if (!Args->TryGetObjectField(TEXT("state"), StateObjPtr) || !StateObjPtr || !StateObjPtr->IsValid())
			{
				R.bOk = false;
				R.ErrorCode = TEXT("INVALID_ARGS");
				R.ErrorMessage = TEXT("missing 'state' object");
				return R;
			}

			TSharedPtr<FJsonObject> NewStateJson;
			TSharedPtr<FJsonObject> StateInput = *StateObjPtr;

			TSharedPtr<FJsonObject> ResultObj;
			FString ErrorCode, ErrorMessage;
			bool bResolved = false;

			MyikaMCP::RunOnGameThreadVoid([&]()
			{
				UMyikaSubsystem* Sub = Ctx.ResolveSubsystem();
				if (!Sub)
				{
					return;
				}
				bResolved = true;

				FMyikaGlobalState Patched = Sub->GetGlobalState();
				FString ParseError;
				if (!MyikaMCP::JsonToState(StateInput, Patched, ParseError))
				{
					ErrorCode = TEXT("INVALID_ARGS");
					ErrorMessage = ParseError;
					return;
				}
				Sub->SetGlobalState(Patched);

				ResultObj = MakeShared<FJsonObject>();
				ResultObj->SetObjectField(TEXT("state"), MyikaMCP::StateToJson(Sub->GetGlobalState()));
			});

			if (!bResolved)
			{
				R.bOk = false;
				R.ErrorCode = TEXT("NO_SUBSYSTEM");
				R.ErrorMessage = TEXT("UMyikaSubsystem unavailable (start PIE or run game)");
				return R;
			}
			if (!ErrorCode.IsEmpty())
			{
				R.bOk = false;
				R.ErrorCode = ErrorCode;
				R.ErrorMessage = ErrorMessage;
				return R;
			}

			R.bOk = true;
			R.Result = ResultObj;
			return R;
		};
	}
}
