// Copyright (c) Myika AI. All rights reserved.

using UnrealBuildTool;

public class MyikaWater : ModuleRules
{
	public MyikaWater(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		IWYUSupport = IWYUSupport.Full;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"MyikaCore",
			"Water",
			"WaterAdvanced",
			"Buoyancy",
			"Niagara",
			"EnhancedInput"
		});

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"InputCore",
			"PhysicsCore"
		});
	}
}
