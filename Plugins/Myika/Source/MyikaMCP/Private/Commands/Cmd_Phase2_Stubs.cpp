// Copyright (c) Myika AI. All rights reserved.

#include "MyikaMCPCommands.h"
#include "Dom/JsonObject.h"

namespace MyikaMCP::Commands
{
	FMyikaMCPHandlerFn MakePhase2StubHandler(const FString& FeatureName)
	{
		FString Name = FeatureName;
		return [Name](const FMyikaMCPCommandContext&, const TSharedPtr<FJsonObject>&) -> FMyikaMCPHandlerResult
		{
			FMyikaMCPHandlerResult R;
			R.bOk = false;
			R.ErrorCode = TEXT("NOT_IMPLEMENTED");
			R.ErrorMessage = FString::Printf(TEXT("'%s' is a Phase 2 stub; the corresponding Myika vertical has not shipped yet"), *Name);
			return R;
		};
	}
}
