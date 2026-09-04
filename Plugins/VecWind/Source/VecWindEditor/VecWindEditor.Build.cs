using UnrealBuildTool;

public class VecWindEditor : ModuleRules
{
    public VecWindEditor(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PrivateDependencyModuleNames.AddRange(new string[]
        {
            "Core",
            "CoreUObject",
            "Engine",
            "VecWind",
            "UnrealEd",
            "AssetTools",
            "Projects",
            "Slate",
            "SlateCore"
        });
    }
}