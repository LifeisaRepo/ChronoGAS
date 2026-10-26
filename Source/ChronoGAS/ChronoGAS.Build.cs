// Copyright (c) 2026 Sanjyot Dahale. All rights reserved.

using UnrealBuildTool;

public class ChronoGAS : ModuleRules
{
	public ChronoGAS(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput", "GameplayAbilities", "UMG" });

		PrivateDependencyModuleNames.AddRange(new string[] { "GameplayTags", "GameplayTasks", "AIModule", "Slate", "SlateCore" });
	}
}
