// Copyright (c) Myika AI. All rights reserved.

using UnrealBuildTool;

public class MyikaMCP : ModuleRules
{
	public MyikaMCP(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		IWYUSupport = IWYUSupport.Full;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"DeveloperSettings",
			"MyikaCore"
		});

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"Sockets",
			"Networking",
			"Json",
			"JsonUtilities",
			"Slate",
			"SlateCore",
			"InputCore",
			"Projects",
			"ToolMenus",
			"UnrealEd",
			"Settings"
		});
	}
}
