// Copyright (c) Myika AI. All rights reserved.

#include "MyikaMCPCommands.h"
#include "MyikaMCPSettings.h"
#include "Dom/JsonObject.h"

namespace MyikaMCP::Commands
{
	namespace
	{
		TSharedRef<FJsonObject> EmptyObjectSchema()
		{
			TSharedRef<FJsonObject> Schema = MakeShared<FJsonObject>();
			Schema->SetStringField(TEXT("type"), TEXT("object"));
			Schema->SetObjectField(TEXT("properties"), MakeShared<FJsonObject>());
			return Schema;
		}

		TSharedRef<FJsonObject> NumberProperty(double Min, double Max, const FString& Description)
		{
			TSharedRef<FJsonObject> Prop = MakeShared<FJsonObject>();
			Prop->SetStringField(TEXT("type"), TEXT("number"));
			Prop->SetNumberField(TEXT("minimum"), Min);
			Prop->SetNumberField(TEXT("maximum"), Max);
			Prop->SetStringField(TEXT("description"), Description);
			return Prop;
		}

		TSharedRef<FJsonObject> NumberAny(const FString& Description)
		{
			TSharedRef<FJsonObject> Prop = MakeShared<FJsonObject>();
			Prop->SetStringField(TEXT("type"), TEXT("number"));
			Prop->SetStringField(TEXT("description"), Description);
			return Prop;
		}

		TSharedRef<FJsonObject> StringEnum(const TArray<FString>& Values, const FString& Description)
		{
			TSharedRef<FJsonObject> Prop = MakeShared<FJsonObject>();
			Prop->SetStringField(TEXT("type"), TEXT("string"));
			TArray<TSharedPtr<FJsonValue>> Enum;
			for (const FString& V : Values)
			{
				Enum.Add(MakeShared<FJsonValueString>(V));
			}
			Prop->SetArrayField(TEXT("enum"), Enum);
			Prop->SetStringField(TEXT("description"), Description);
			return Prop;
		}

		TSharedRef<FJsonObject> ObjectSchema(
			const TArray<TPair<FString, TSharedRef<FJsonObject>>>& Properties,
			const TArray<FString>& Required)
		{
			TSharedRef<FJsonObject> Schema = MakeShared<FJsonObject>();
			Schema->SetStringField(TEXT("type"), TEXT("object"));

			TSharedRef<FJsonObject> Props = MakeShared<FJsonObject>();
			for (const auto& Pair : Properties)
			{
				Props->SetObjectField(Pair.Key, Pair.Value);
			}
			Schema->SetObjectField(TEXT("properties"), Props);

			TArray<TSharedPtr<FJsonValue>> Req;
			for (const FString& R : Required)
			{
				Req.Add(MakeShared<FJsonValueString>(R));
			}
			Schema->SetArrayField(TEXT("required"), Req);
			return Schema;
		}

		const TArray<FString>& WeatherEnum()
		{
			static const TArray<FString> Values = {
				TEXT("Clear"), TEXT("PartlyCloudy"), TEXT("Overcast"),
				TEXT("LightRain"), TEXT("HeavyRain"), TEXT("Thunderstorm"),
				TEXT("Snow"), TEXT("Blizzard"), TEXT("Foggy"), TEXT("SandStorm")
			};
			return Values;
		}

		// State sub-schema reused by set_global_state.
		TSharedRef<FJsonObject> StateObjectSchema()
		{
			return ObjectSchema(
				{
					{ TEXT("time_of_day"), NumberProperty(0, 24, TEXT("Hours, 0..24")) },
					{ TEXT("weather"), StringEnum(WeatherEnum(), TEXT("Discrete weather state")) },
					{ TEXT("wetness"), NumberProperty(0, 1, TEXT("Surface wetness 0..1")) },
					{ TEXT("snow_coverage"), NumberProperty(0, 1, TEXT("Snow coverage 0..1")) },
					{ TEXT("storm_intensity"), NumberProperty(0, 1, TEXT("Storm intensity 0..1")) },
					{ TEXT("temperature"), NumberAny(TEXT("Ambient temperature in Celsius")) },
				},
				{} /* no required fields — partial patch */);
		}

		void RegisterWithMetadata(
			const FString& Name,
			const FString& Description,
			const TSharedRef<FJsonObject>& Schema,
			FMyikaMCPHandlerFn Handler)
		{
			FMyikaMCPCommandRegistry& R = FMyikaMCPCommandRegistry::Get();
			R.Register(Name, MoveTemp(Handler));
			R.RegisterDescription(Name, Description);
			R.RegisterSchema(Name, Schema);
		}
	}

	void RegisterAll()
	{
		RegisterWithMetadata(
			TEXT("ping"),
			TEXT("Health check; returns plugin and engine versions. Use first to confirm connectivity."),
			EmptyObjectSchema(),
			MakePingHandler());

		RegisterWithMetadata(
			TEXT("get_global_state"),
			TEXT("Return the current Myika global state (time, weather, wind, surface state)."),
			EmptyObjectSchema(),
			MakeGetGlobalStateHandler());

		RegisterWithMetadata(
			TEXT("set_global_state"),
			TEXT("Patch one or more fields of the Myika global state. Send only the fields you want to change inside `state`; omitted fields keep their current value."),
			ObjectSchema(
				{ { TEXT("state"), StateObjectSchema() } },
				{ TEXT("state") }),
			MakeSetGlobalStateHandler());

		RegisterWithMetadata(
			TEXT("set_time_of_day"),
			TEXT("Set the in-game time of day. Hours is 0..24 (decimals OK; 17.5 = 5:30pm)."),
			ObjectSchema(
				{ { TEXT("hours"), NumberProperty(0, 24, TEXT("Game-time hours, 0..24")) } },
				{ TEXT("hours") }),
			MakeSetTimeOfDayHandler());

		RegisterWithMetadata(
			TEXT("set_weather"),
			TEXT("Set the discrete weather state."),
			ObjectSchema(
				{ { TEXT("weather"), StringEnum(WeatherEnum(), TEXT("Discrete weather state")) } },
				{ TEXT("weather") }),
			MakeSetWeatherHandler());

		RegisterWithMetadata(
			TEXT("set_storm_intensity"),
			TEXT("Continuous storm intensity 0..1; drives sky/cloud morphology."),
			ObjectSchema(
				{ { TEXT("value"), NumberProperty(0, 1, TEXT("Storm intensity 0..1")) } },
				{ TEXT("value") }),
			MakeSetStormIntensityHandler());

		RegisterWithMetadata(
			TEXT("set_wind"),
			TEXT("Set the global wind vector in cm/s (Unreal world units; 100 cm/s ~= 1 m/s)."),
			ObjectSchema(
				{
					{ TEXT("x"), NumberAny(TEXT("Wind X in cm/s")) },
					{ TEXT("y"), NumberAny(TEXT("Wind Y in cm/s")) },
					{ TEXT("z"), NumberAny(TEXT("Wind Z in cm/s")) },
				},
				{ TEXT("x"), TEXT("y"), TEXT("z") }),
			MakeSetWindHandler());

		RegisterWithMetadata(
			TEXT("set_wetness"),
			TEXT("Set surface wetness 0..1 (drives material macros)."),
			ObjectSchema(
				{ { TEXT("value"), NumberProperty(0, 1, TEXT("Wetness 0..1")) } },
				{ TEXT("value") }),
			MakeSetWetnessHandler());

		RegisterWithMetadata(
			TEXT("set_snow_coverage"),
			TEXT("Set snow coverage 0..1 (drives material macros)."),
			ObjectSchema(
				{ { TEXT("value"), NumberProperty(0, 1, TEXT("Snow coverage 0..1")) } },
				{ TEXT("value") }),
			MakeSetSnowCoverageHandler());

		RegisterWithMetadata(
			TEXT("set_temperature"),
			TEXT("Set ambient temperature in degrees Celsius."),
			ObjectSchema(
				{ { TEXT("celsius"), NumberAny(TEXT("Temperature in Celsius")) } },
				{ TEXT("celsius") }),
			MakeSetTemperatureHandler());

		const UMyikaMCPSettings* Settings = GetDefault<UMyikaMCPSettings>();
		if (Settings && Settings->bAllowPhase2Stubs)
		{
			RegisterWithMetadata(
				TEXT("pcg_regen"),
				TEXT("(Phase 2) Trigger PCG biome regeneration. Currently returns NOT_IMPLEMENTED."),
				ObjectSchema(
					{ { TEXT("region"), [](){ TSharedRef<FJsonObject> P = MakeShared<FJsonObject>(); P->SetStringField(TEXT("type"), TEXT("string")); return P; }() } },
					{}),
				MakePhase2StubHandler(TEXT("pcg_regen")));

			RegisterWithMetadata(
				TEXT("spawn_npc"),
				TEXT("(Phase 2) Spawn an NPC at a location. Currently returns NOT_IMPLEMENTED."),
				ObjectSchema(
					{
						{ TEXT("class"), [](){ TSharedRef<FJsonObject> P = MakeShared<FJsonObject>(); P->SetStringField(TEXT("type"), TEXT("string")); return P; }() },
						{ TEXT("x"), NumberAny(TEXT("X cm")) },
						{ TEXT("y"), NumberAny(TEXT("Y cm")) },
						{ TEXT("z"), NumberAny(TEXT("Z cm")) },
					},
					{ TEXT("class"), TEXT("x"), TEXT("y"), TEXT("z") }),
				MakePhase2StubHandler(TEXT("spawn_npc")));

			RegisterWithMetadata(
				TEXT("lightning_strike"),
				TEXT("(Phase 2) Trigger a lightning strike at a location. Currently returns NOT_IMPLEMENTED."),
				ObjectSchema(
					{
						{ TEXT("x"), NumberAny(TEXT("X cm")) },
						{ TEXT("y"), NumberAny(TEXT("Y cm")) },
						{ TEXT("z"), NumberAny(TEXT("Z cm")) },
					},
					{ TEXT("x"), TEXT("y"), TEXT("z") }),
				MakePhase2StubHandler(TEXT("lightning_strike")));
		}
	}

	void UnregisterAll()
	{
		FMyikaMCPCommandRegistry::Get().Clear();
	}
}
