#include "VecWindEditor.h"

#include "AssetToolsModule.h"
#include "AssetTypeActions_WindVectorField.h"
#include "IAssetTools.h"
#include "Brushes/SlateImageBrush.h"
#include "Interfaces/IPluginManager.h"
#include "Misc/Paths.h"
#include "Styling/SlateStyle.h"
#include "Styling/SlateStyleRegistry.h"

IMPLEMENT_MODULE(FVecWindEditorModule, VecWindEditor)

void FVecWindEditorModule::StartupModule()
{
    const TSharedPtr<IPlugin> Plugin =
        IPluginManager::Get().FindPlugin(TEXT("VecWind"));

    check(Plugin.IsValid());

    StyleSet = MakeShared<FSlateStyleSet>(TEXT("VecWindEditorStyle"));
    StyleSet->SetContentRoot(
        FPaths::Combine(Plugin->GetBaseDir(), TEXT("Resources")));

    StyleSet->Set(
        TEXT("ClassIcon.WindVectorField"),
        new FSlateImageBrush(
            StyleSet->RootToContentDir(
                TEXT("Icons/VecWind_Logo_16"),
                TEXT(".png")),
            FVector2D(16.0f, 16.0f)));

    StyleSet->Set(
        TEXT("ClassThumbnail.WindVectorField"),
        new FSlateImageBrush(
            StyleSet->RootToContentDir(
                TEXT("Icons/VecWind_Logo_64"),
                TEXT(".png")),
            FVector2D(64.0f, 64.0f)));

    FSlateStyleRegistry::RegisterSlateStyle(*StyleSet);

    IAssetTools& AssetTools =
        FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();

    TSharedPtr<IAssetTypeActions> WindFieldActions =
        MakeShared<FAssetTypeActions_WindVectorField>();

    AssetTools.RegisterAssetTypeActions(WindFieldActions.ToSharedRef());
    RegisteredAssetTypeActions.Add(WindFieldActions);
}

void FVecWindEditorModule::ShutdownModule()
{
    if (FModuleManager::Get().IsModuleLoaded("AssetTools"))
    {
        IAssetTools& AssetTools =
            FModuleManager::GetModuleChecked<FAssetToolsModule>("AssetTools").Get();

        for (const TSharedPtr<IAssetTypeActions>& Actions : RegisteredAssetTypeActions)
        {
            AssetTools.UnregisterAssetTypeActions(Actions.ToSharedRef());
        }
    }

    RegisteredAssetTypeActions.Empty();

    if (StyleSet.IsValid())
    {
        FSlateStyleRegistry::UnRegisterSlateStyle(*StyleSet);
        StyleSet.Reset();
    }
}