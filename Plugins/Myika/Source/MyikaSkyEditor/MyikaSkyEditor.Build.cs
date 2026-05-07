// Copyright (c) Myika AI. All rights reserved.

using UnrealBuildTool;

public class MyikaSkyEditor : ModuleRules
{
	public MyikaSkyEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		IWYUSupport = IWYUSupport.Full;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"UMG",
			"MyikaCore",
			"MyikaSky"
		});

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"Blutility",
			"Slate",
			"SlateCore",
			"UnrealEd"
		});
	}
}
