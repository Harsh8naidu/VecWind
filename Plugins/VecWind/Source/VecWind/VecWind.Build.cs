using UnrealBuildTool;
using System.IO;

public class VecWind : ModuleRules
{
    public VecWind(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new string[]
        {
            "Core",
            "CoreUObject",
            "Engine",
            "Niagara",
            "NiagaraCore",
            "NiagaraShader",
            "RenderCore",
            "RHI"
        });

        PrivateDependencyModuleNames.AddRange(new string[]
        {
            "Projects"
        });

        PublicIncludePaths.Add(
            Path.Combine(ModuleDirectory, "..", "ThirdParty", "FastNoise"));
    }
}