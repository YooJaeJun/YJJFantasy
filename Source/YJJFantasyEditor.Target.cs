using UnrealBuildTool;

public class YJJFantasyEditorTarget : TargetRules
{
	public YJJFantasyEditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.V7;
		IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_8;
		ExtraModuleNames.Add("YJJFantasy");
	}
}
