#include "VecWind.h"
#include "Interfaces/IPluginManager.h"
#include "Misc/Paths.h"
#include "ShaderCore.h"

IMPLEMENT_MODULE(FVecWindModule, VecWind)

void FVecWindModule::StartupModule()
{
    const TSharedPtr<IPlugin> Plugin = IPluginManager::Get().FindPlugin(TEXT("VecWind"));

    check(Plugin.IsValid());

    AddShaderSourceDirectoryMapping(
        TEXT("/Plugins/VecWind"),
        FPaths::Combine(Plugin->GetBaseDir(), TEXT("Shaders"))
    );
}