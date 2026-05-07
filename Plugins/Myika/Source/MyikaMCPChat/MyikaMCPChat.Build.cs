// Copyright (c) Myika AI. All rights reserved.

using UnrealBuildTool;

public class MyikaMCPChat : ModuleRules
{
	public MyikaMCPChat(ReadOnlyTargetRules Target) : base(Target)
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
			"MyikaMCP",
			"HTTP",
			"Json",
			"JsonUtilities",
			"Slate",
			"SlateCore",
			"InputCore",
			"Projects",
			"ToolMenus",
			"UnrealEd",
			"WorkspaceMenuStructure",
			"Settings"
		});
	}
}
