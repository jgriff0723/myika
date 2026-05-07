// Copyright (c) Myika AI. All rights reserved.

#include "MyikaMCPCommands.h"
#include "MyikaMCPGameThread.h"
#include "MyikaMCPJsonHelpers.h"
#include "MyikaSubsystem.h"
#include "Dom/JsonObject.h"

namespace MyikaMCP::Commands
{
	FMyikaMCPHandlerFn MakeSetWindHandler()
	{
		return [](const FMyikaMCPCommandContext& Ctx, const TSharedPtr<FJsonObject>& Args) -> FMyikaMCPHandlerResult
		{
			FMyikaMCPHandlerResult R;

			double X = 0, Y = 0, Z = 0;
			if (!Args->TryGetNumberField(TEXT("x"), X) ||
				!Args->TryGetNumberField(TEXT("y"), Y) ||
				!Args->TryGetNumberField(TEXT("z"), Z))
			{
				R.bOk = false;
				R.ErrorCode = TEXT("INVALID_ARGS");
				R.ErrorMessage = TEXT("missing numeric 'x', 'y', 'z'");
				return R;
			}

			TSharedPtr<FJsonObject> StateJson;
			bool bResolved = false;
			MyikaMCP::RunOnGameThreadVoid([&]()
			{
				if (UMyikaSubsystem* Sub = Ctx.ResolveSubsystem())
				{
					bResolved = true;
					FMyikaGlobalState State = Sub->GetGlobalState();
					State.WindVector = FVector(X, Y, Z);
					Sub->SetGlobalState(State);
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
}
