// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class myikai_plugin : ModuleRules
{
	public myikai_plugin(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
			"AIModule",
			"StateTreeModule",
			"GameplayStateTreeModule",
			"UMG",
			"Slate"
		});

		PrivateDependencyModuleNames.AddRange(new string[] { });

		PublicIncludePaths.AddRange(new string[] {
			"myikai_plugin",
			"myikai_plugin/Variant_Platforming",
			"myikai_plugin/Variant_Platforming/Animation",
			"myikai_plugin/Variant_Combat",
			"myikai_plugin/Variant_Combat/AI",
			"myikai_plugin/Variant_Combat/Animation",
			"myikai_plugin/Variant_Combat/Gameplay",
			"myikai_plugin/Variant_Combat/Interfaces",
			"myikai_plugin/Variant_Combat/UI",
			"myikai_plugin/Variant_SideScrolling",
			"myikai_plugin/Variant_SideScrolling/AI",
			"myikai_plugin/Variant_SideScrolling/Gameplay",
			"myikai_plugin/Variant_SideScrolling/Interfaces",
			"myikai_plugin/Variant_SideScrolling/UI"
		});

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
