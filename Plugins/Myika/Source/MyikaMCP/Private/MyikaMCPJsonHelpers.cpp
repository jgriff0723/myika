// Copyright (c) Myika AI. All rights reserved.

#include "MyikaMCPJsonHelpers.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonWriter.h"

namespace MyikaMCP
{
	TSharedRef<FJsonObject> BuildOk(const FString& Id, const TSharedPtr<FJsonObject>& Result)
	{
		TSharedRef<FJsonObject> Envelope = MakeShared<FJsonObject>();
		Envelope->SetStringField(TEXT("id"), Id);
		Envelope->SetBoolField(TEXT("ok"), true);
		Envelope->SetObjectField(TEXT("result"), Result.IsValid() ? Result : MakeShared<FJsonObject>());
		return Envelope;
	}

	TSharedRef<FJsonObject> BuildError(const FString& Id, const FString& Code, const FString& Message)
	{
		TSharedRef<FJsonObject> Envelope = MakeShared<FJsonObject>();
		Envelope->SetStringField(TEXT("id"), Id);
		Envelope->SetBoolField(TEXT("ok"), false);

		TSharedRef<FJsonObject> ErrorObj = MakeShared<FJsonObject>();
		ErrorObj->SetStringField(TEXT("code"), Code);
		ErrorObj->SetStringField(TEXT("message"), Message);
		Envelope->SetObjectField(TEXT("error"), ErrorObj);
		return Envelope;
	}

	bool SerializeJson(const TSharedRef<FJsonObject>& Object, TArray<uint8>& OutBytes)
	{
		FString Text;
		TSharedRef<TJsonWriter<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>> Writer =
			TJsonWriterFactory<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>::Create(&Text);
		if (!FJsonSerializer::Serialize(Object, Writer))
		{
			return false;
		}
		FTCHARToUTF8 Utf8(*Text);
		OutBytes.SetNumUninitialized(Utf8.Length());
		FMemory::Memcpy(OutBytes.GetData(), Utf8.Get(), Utf8.Length());
		return true;
	}

	bool DeserializeJson(const uint8* Bytes, int32 NumBytes, TSharedPtr<FJsonObject>& OutObject)
	{
		if (Bytes == nullptr || NumBytes <= 0)
		{
			return false;
		}
		FUTF8ToTCHAR Tchars(reinterpret_cast<const ANSICHAR*>(Bytes), NumBytes);
		const FString Text(Tchars.Length(), Tchars.Get());
		TSharedRef<TJsonReader<TCHAR>> Reader = TJsonReaderFactory<TCHAR>::Create(Text);
		return FJsonSerializer::Deserialize(Reader, OutObject) && OutObject.IsValid();
	}

	const TCHAR* WeatherToString(EMyikaWeatherState Weather)
	{
		switch (Weather)
		{
		case EMyikaWeatherState::Clear:        return TEXT("Clear");
		case EMyikaWeatherState::PartlyCloudy: return TEXT("PartlyCloudy");
		case EMyikaWeatherState::Overcast:     return TEXT("Overcast");
		case EMyikaWeatherState::LightRain:    return TEXT("LightRain");
		case EMyikaWeatherState::HeavyRain:    return TEXT("HeavyRain");
		case EMyikaWeatherState::Thunderstorm: return TEXT("Thunderstorm");
		case EMyikaWeatherState::Snow:         return TEXT("Snow");
		case EMyikaWeatherState::Blizzard:     return TEXT("Blizzard");
		case EMyikaWeatherState::Foggy:        return TEXT("Foggy");
		case EMyikaWeatherState::SandStorm:    return TEXT("SandStorm");
		}
		return TEXT("Clear");
	}

	bool StringToWeather(const FString& Name, EMyikaWeatherState& OutWeather)
	{
		static const TMap<FString, EMyikaWeatherState> Map = {
			{ TEXT("Clear"),        EMyikaWeatherState::Clear },
			{ TEXT("PartlyCloudy"), EMyikaWeatherState::PartlyCloudy },
			{ TEXT("Overcast"),     EMyikaWeatherState::Overcast },
			{ TEXT("LightRain"),    EMyikaWeatherState::LightRain },
			{ TEXT("HeavyRain"),    EMyikaWeatherState::HeavyRain },
			{ TEXT("Thunderstorm"), EMyikaWeatherState::Thunderstorm },
			{ TEXT("Snow"),         EMyikaWeatherState::Snow },
			{ TEXT("Blizzard"),     EMyikaWeatherState::Blizzard },
			{ TEXT("Foggy"),        EMyikaWeatherState::Foggy },
			{ TEXT("SandStorm"),    EMyikaWeatherState::SandStorm }
		};
		if (const EMyikaWeatherState* Found = Map.Find(Name))
		{
			OutWeather = *Found;
			return true;
		}
		return false;
	}

	const TCHAR* PartOfDayToString(EMyikaPartOfDay Part)
	{
		switch (Part)
		{
		case EMyikaPartOfDay::Dawn:      return TEXT("Dawn");
		case EMyikaPartOfDay::Morning:   return TEXT("Morning");
		case EMyikaPartOfDay::Noon:      return TEXT("Noon");
		case EMyikaPartOfDay::Afternoon: return TEXT("Afternoon");
		case EMyikaPartOfDay::Dusk:      return TEXT("Dusk");
		case EMyikaPartOfDay::Night:     return TEXT("Night");
		}
		return TEXT("Noon");
	}

	static TSharedRef<FJsonObject> VectorToJson(const FVector& V)
	{
		TSharedRef<FJsonObject> Obj = MakeShared<FJsonObject>();
		Obj->SetNumberField(TEXT("x"), V.X);
		Obj->SetNumberField(TEXT("y"), V.Y);
		Obj->SetNumberField(TEXT("z"), V.Z);
		return Obj;
	}

	static bool JsonToVector(const TSharedPtr<FJsonObject>& Json, FVector& Out)
	{
		double X = 0, Y = 0, Z = 0;
		if (!Json->TryGetNumberField(TEXT("x"), X)) return false;
		if (!Json->TryGetNumberField(TEXT("y"), Y)) return false;
		if (!Json->TryGetNumberField(TEXT("z"), Z)) return false;
		Out = FVector(X, Y, Z);
		return true;
	}

	TSharedRef<FJsonObject> StateToJson(const FMyikaGlobalState& State)
	{
		TSharedRef<FJsonObject> Obj = MakeShared<FJsonObject>();
		Obj->SetNumberField(TEXT("time_of_day"), State.TimeOfDay);
		Obj->SetStringField(TEXT("part_of_day"), PartOfDayToString(State.PartOfDay));
		Obj->SetStringField(TEXT("weather"), WeatherToString(State.Weather));
		Obj->SetObjectField(TEXT("sun_direction"), VectorToJson(State.SunDirection));
		Obj->SetObjectField(TEXT("moon_direction"), VectorToJson(State.MoonDirection));
		Obj->SetObjectField(TEXT("wind_vector"), VectorToJson(State.WindVector));
		Obj->SetNumberField(TEXT("wetness"), State.Wetness);
		Obj->SetNumberField(TEXT("snow_coverage"), State.SnowCoverage);
		Obj->SetNumberField(TEXT("temperature"), State.Temperature);
		Obj->SetNumberField(TEXT("storm_intensity"), State.StormIntensity);
		return Obj;
	}

	bool JsonToState(const TSharedPtr<FJsonObject>& Json, FMyikaGlobalState& OutState, FString& OutError)
	{
		if (!Json.IsValid())
		{
			OutError = TEXT("state object missing");
			return false;
		}

		double TimeOfDay = OutState.TimeOfDay;
		if (Json->TryGetNumberField(TEXT("time_of_day"), TimeOfDay))
		{
			if (TimeOfDay < 0.0 || TimeOfDay > 24.0)
			{
				OutError = TEXT("time_of_day must be 0..24");
				return false;
			}
			OutState.TimeOfDay = static_cast<float>(TimeOfDay);
		}

		FString WeatherStr;
		if (Json->TryGetStringField(TEXT("weather"), WeatherStr))
		{
			EMyikaWeatherState W;
			if (!StringToWeather(WeatherStr, W))
			{
				OutError = FString::Printf(TEXT("unknown weather '%s'"), *WeatherStr);
				return false;
			}
			OutState.Weather = W;
		}

		const TSharedPtr<FJsonObject>* SunObj;
		if (Json->TryGetObjectField(TEXT("sun_direction"), SunObj) && SunObj && SunObj->IsValid())
		{
			FVector V;
			if (!JsonToVector(*SunObj, V)) { OutError = TEXT("bad sun_direction"); return false; }
			OutState.SunDirection = V;
		}

		const TSharedPtr<FJsonObject>* MoonObj;
		if (Json->TryGetObjectField(TEXT("moon_direction"), MoonObj) && MoonObj && MoonObj->IsValid())
		{
			FVector V;
			if (!JsonToVector(*MoonObj, V)) { OutError = TEXT("bad moon_direction"); return false; }
			OutState.MoonDirection = V;
		}

		const TSharedPtr<FJsonObject>* WindObj;
		if (Json->TryGetObjectField(TEXT("wind_vector"), WindObj) && WindObj && WindObj->IsValid())
		{
			FVector V;
			if (!JsonToVector(*WindObj, V)) { OutError = TEXT("bad wind_vector"); return false; }
			OutState.WindVector = V;
		}

		auto ReadFloat01 = [&](const TCHAR* Field, float& Out) -> bool
		{
			double V = Out;
			if (Json->TryGetNumberField(Field, V))
			{
				if (V < 0.0 || V > 1.0)
				{
					OutError = FString::Printf(TEXT("%s must be 0..1"), Field);
					return false;
				}
				Out = static_cast<float>(V);
			}
			return true;
		};

		if (!ReadFloat01(TEXT("wetness"), OutState.Wetness)) return false;
		if (!ReadFloat01(TEXT("snow_coverage"), OutState.SnowCoverage)) return false;
		if (!ReadFloat01(TEXT("storm_intensity"), OutState.StormIntensity)) return false;

		double Temp = OutState.Temperature;
		if (Json->TryGetNumberField(TEXT("temperature"), Temp))
		{
			OutState.Temperature = static_cast<float>(Temp);
		}

		return true;
	}
}
