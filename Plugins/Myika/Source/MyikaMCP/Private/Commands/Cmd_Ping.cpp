// Copyright (c) Myika AI. All rights reserved.

#include "MyikaMCPCommands.h"
#include "Dom/JsonObject.h"
#include "Misc/App.h"
#include "Misc/EngineVersion.h"

namespace MyikaMCP::Commands
{
	FMyikaMCPHandlerFn MakePingHandler()
	{
		return [](const FMyikaMCPCommandContext&, const TSharedPtr<FJsonObject>&) -> FMyikaMCPHandlerResult
		{
			FMyikaMCPHandlerResult R;
			R.bOk = true;
			R.Result = MakeShared<FJsonObject>();
			R.Result->SetBoolField(TEXT("pong"), true);
			R.Result->SetStringField(TEXT("plugin_version"), TEXT("0.1.0"));
			R.Result->SetStringField(TEXT("engine_version"), FEngineVersion::Current().ToString());
			R.Result->SetStringField(TEXT("project_name"), FApp::GetProjectName());
			return R;
		};
	}
}
