// Copyright (c) Myika AI. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Dom/JsonObject.h"
#include "MyikaTypes.h"

namespace MyikaMCP
{
	/** Build a successful response envelope with given id + result payload. */
	TSharedRef<FJsonObject> BuildOk(const FString& Id, const TSharedPtr<FJsonObject>& Result);

	/** Build an error response envelope. */
	TSharedRef<FJsonObject> BuildError(const FString& Id, const FString& Code, const FString& Message);

	/** Serialize an FJsonObject to a UTF-8 byte array (no length prefix). */
	bool SerializeJson(const TSharedRef<FJsonObject>& Object, TArray<uint8>& OutBytes);

	/** Parse a UTF-8 byte buffer into an FJsonObject. */
	bool DeserializeJson(const uint8* Bytes, int32 NumBytes, TSharedPtr<FJsonObject>& OutObject);

	/** FMyikaGlobalState <-> FJsonObject. */
	TSharedRef<FJsonObject> StateToJson(const FMyikaGlobalState& State);
	bool JsonToState(const TSharedPtr<FJsonObject>& Json, FMyikaGlobalState& OutState, FString& OutError);

	/** EMyikaWeatherState <-> FString. */
	const TCHAR* WeatherToString(EMyikaWeatherState Weather);
	bool StringToWeather(const FString& Name, EMyikaWeatherState& OutWeather);

	/** EMyikaPartOfDay -> FString (read-only convenience). */
	const TCHAR* PartOfDayToString(EMyikaPartOfDay Part);
}
