using UnrealBuildTool;

public class YJJFantasyTarget : TargetRules
{
	public YJJFantasyTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		DefaultBuildSettings = BuildSettingsVersion.V7;
		IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_8;
		ExtraModuleNames.Add("YJJFantasy");
	}
}
