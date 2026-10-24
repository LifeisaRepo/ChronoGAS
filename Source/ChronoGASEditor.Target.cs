// Copyright (c) 2026 Sanjyot Dahale. All rights reserved.

using UnrealBuildTool;
using System.Collections.Generic;

public class ChronoGASEditorTarget : TargetRules
{
	public ChronoGASEditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.V4;
		IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_3;
		ExtraModuleNames.Add("ChronoGAS");
	}
}
