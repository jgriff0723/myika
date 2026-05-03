// Copyright (c) Myika AI. All rights reserved.

using UnrealBuildTool;

public class MyikaCore : ModuleRules
{
	public MyikaCore(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		IWYUSupport = IWYUSupport.Full;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"DeveloperSettings"
		});

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"InputCore",
			"Slate",
			"SlateCore",
			"UMG",
			"RenderCore",
			"RHI"
		});
	}
}
