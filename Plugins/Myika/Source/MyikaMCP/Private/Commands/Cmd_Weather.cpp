// Copyright (c) Myika AI. All rights reserved.

#include "MyikaMCPCommands.h"
#include "MyikaMCPGameThread.h"
#include "MyikaMCPJsonHelpers.h"
#include "MyikaSubsystem.h"
#include "Dom/JsonObject.h"

namespace MyikaMCP::Commands
{
	FMyikaMCPHandlerFn MakeSetWeatherHandler()
	{
		return [](const FMyikaMCPCommandContext& Ctx, const TSharedPtr<FJsonObject>& Args) -> FMyikaMCPHandlerResult
		{
			FMyikaMCPHandlerResult R;

			FString WeatherStr;
			if (!Args->TryGetStringField(TEXT("weather"), WeatherStr) || WeatherStr.IsEmpty())
			{
				R.bOk = false;
				R.ErrorCode = TEXT("INVALID_ARGS");
				R.ErrorMessage = TEXT("missing 'weather' string");
				return R;
			}

			EMyikaWeatherState Weather;
			if (!MyikaMCP::StringToWeather(WeatherStr, Weather))
			{
				R.bOk = false;
				R.ErrorCode = TEXT("INVALID_ARGS");
				R.ErrorMessage = FString::Printf(TEXT("unknown weather '%s' (valid: Clear, PartlyCloudy, Overcast, LightRain, HeavyRain, Thunderstorm, Snow, Blizzard, Foggy, SandStorm)"), *WeatherStr);
				return R;
			}

			TSharedPtr<FJsonObject> StateJson;
			bool bResolved = false;
			MyikaMCP::RunOnGameThreadVoid([&]()
			{
				if (UMyikaSubsystem* Sub = Ctx.ResolveSubsystem())
				{
					bResolved = true;
					Sub->SetWeather(Weather);
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

	FMyikaMCPHandlerFn MakeSetStormIntensityHandler()
	{
		return [](const FMyikaMCPCommandContext& Ctx, const TSharedPtr<FJsonObject>& Args) -> FMyikaMCPHandlerResult
		{
			FMyikaMCPHandlerResult R;

			double Value = 0.0;
			if (!Args->TryGetNumberField(TEXT("value"), Value))
			{
				R.bOk = false;
				R.ErrorCode = TEXT("INVALID_ARGS");
				R.ErrorMessage = TEXT("missing numeric 'value'");
				return R;
			}
			if (Value < 0.0 || Value > 1.0)
			{
				R.bOk = false;
				R.ErrorCode = TEXT("INVALID_ARGS");
				R.ErrorMessage = TEXT("'value' must be 0..1");
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
					State.StormIntensity = static_cast<float>(Value);
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
