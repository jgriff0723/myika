// Copyright (c) Myika AI. All rights reserved.

using UnrealBuildTool;

public class MyikaSky : ModuleRules
{
	public MyikaSky(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		IWYUSupport = IWYUSupport.Full;

		PublicDependencyModuleNames.AddRange(new[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"MyikaCore",
			"SunPosition"
		});
	}
}
