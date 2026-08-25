// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class s1mpleFps : ModuleRules
{
	public s1mpleFps(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicIncludePaths.Add(ModuleDirectory);
		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput", "AIModule", "UMG", "GameplayTasks", "NavigationSystem","OnlineSubsystem","OnlineSubsystemUtils","OnlineSubsystemSteam","SlateCore","Niagara","StreamlineReflexBlueprint"});
	}
}
