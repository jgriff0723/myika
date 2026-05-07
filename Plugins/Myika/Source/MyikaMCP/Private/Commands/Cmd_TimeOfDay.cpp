// Copyright (c) Myika AI. All rights reserved.

#include "MyikaMCPCommands.h"
#include "MyikaMCPGameThread.h"
#include "MyikaMCPJsonHelpers.h"
#include "MyikaSubsystem.h"
#include "Dom/JsonObject.h"

namespace MyikaMCP::Commands
{
	FMyikaMCPHandlerFn MakeSetTimeOfDayHandler()
	{
		return [](const FMyikaMCPCommandContext& Ctx, const TSharedPtr<FJsonObject>& Args) -> FMyikaMCPHandlerResult
		{
			FMyikaMCPHandlerResult R;

			double Hours = 0.0;
			if (!Args->TryGetNumberField(TEXT("hours"), Hours))
			{
				R.bOk = false;
				R.ErrorCode = TEXT("INVALID_ARGS");
				R.ErrorMessage = TEXT("missing numeric 'hours'");
				return R;
			}
			if (Hours < 0.0 || Hours > 24.0)
			{
				R.bOk = false;
				R.ErrorCode = TEXT("INVALID_ARGS");
				R.ErrorMessage = TEXT("'hours' must be 0..24");
				return R;
			}

			TSharedPtr<FJsonObject> StateJson;
			bool bResolved = false;
			MyikaMCP::RunOnGameThreadVoid([&]()
			{
				if (UMyikaSubsystem* Sub = Ctx.ResolveSubsystem())
				{
					bResolved = true;
					Sub->SetTimeOfDay(static_cast<float>(Hours));
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
