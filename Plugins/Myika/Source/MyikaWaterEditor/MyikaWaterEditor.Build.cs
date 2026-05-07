// Copyright (c) Myika AI. All rights reserved.

using UnrealBuildTool;

public class MyikaWaterEditor : ModuleRules
{
	public MyikaWaterEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		IWYUSupport = IWYUSupport.Full;

		PublicDependencyModuleNames.AddRange(new[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"MyikaWater"
		});

		PrivateDependencyModuleNames.AddRange(new[]
		{
			"UnrealEd"
		});
	}
}
