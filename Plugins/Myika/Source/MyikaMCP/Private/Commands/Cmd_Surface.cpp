// Copyright (c) Myika AI. All rights reserved.

#include "MyikaMCPCommands.h"
#include "MyikaMCPGameThread.h"
#include "MyikaMCPJsonHelpers.h"
#include "MyikaSubsystem.h"
#include "Dom/JsonObject.h"

namespace MyikaMCP::Commands
{
	namespace
	{
		using FApplyFn = TFunction<void(FMyikaGlobalState& State, float Value)>;

		FMyikaMCPHandlerFn MakeNormalizedFloatHandler(const TCHAR* FieldName, bool bClamp01, FApplyFn Apply)
		{
			FString Field = FieldName;
			return [Field, bClamp01, Apply](const FMyikaMCPCommandContext& Ctx, const TSharedPtr<FJsonObject>& Args) -> FMyikaMCPHandlerResult
			{
				FMyikaMCPHandlerResult R;

				double Value = 0.0;
				if (!Args->TryGetNumberField(*Field, Value))
				{
					R.bOk = false;
					R.ErrorCode = TEXT("INVALID_ARGS");
					R.ErrorMessage = FString::Printf(TEXT("missing numeric '%s'"), *Field);
					return R;
				}
				if (bClamp01 && (Value < 0.0 || Value > 1.0))
				{
					R.bOk = false;
					R.ErrorCode = TEXT("INVALID_ARGS");
					R.ErrorMessage = FString::Printf(TEXT("'%s' must be 0..1"), *Field);
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
						Apply(State, static_cast<float>(Value));
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

	FMyikaMCPHandlerFn MakeSetWetnessHandler()
	{
		return MakeNormalizedFloatHandler(TEXT("value"), /*bClamp01=*/true,
			[](FMyikaGlobalState& State, float V) { State.Wetness = V; });
	}

	FMyikaMCPHandlerFn MakeSetSnowCoverageHandler()
	{
		return MakeNormalizedFloatHandler(TEXT("value"), /*bClamp01=*/true,
			[](FMyikaGlobalState& State, float V) { State.SnowCoverage = V; });
	}

	FMyikaMCPHandlerFn MakeSetTemperatureHandler()
	{
		return MakeNormalizedFloatHandler(TEXT("celsius"), /*bClamp01=*/false,
			[](FMyikaGlobalState& State, float V) { State.Temperature = V; });
	}
}
