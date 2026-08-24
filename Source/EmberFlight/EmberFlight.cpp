// Copyright Epic Games, Inc. All Rights Reserved.

#include "EmberFlight.h"
#include "NiagaraDataInterface.h"
#include "NiagaraDataInterfaceWindField.h"
#include "UObject/UObjectIterator.h"
#include "Modules/ModuleManager.h"

#if WITH_EDITOR
#include "IAssetTools.h"
#include "AssetToolsModule.h"
#include "AssetTypeActions_WindVectorField.h"
#endif

IMPLEMENT_PRIMARY_GAME_MODULE(FEmberFlightModule, EmberFlight, "EmberFlight" );

#if WITH_EDITOR
void FEmberFlightModule::StartupModule()
{
    UE_LOG(LogTemp, Warning, TEXT("Found Startup Module"));
    IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();

    AssetTools.RegisterAssetTypeActions(MakeShareable(new FAssetTypeActions_WindVectorField));
}

void FEmberFlightModule::ShutdownModule()
{
    
}

void ListAllNiagaraInterfaces()
{
    static const FName TempClassName = UNiagaraDataInterfaceWindField::StaticClass()->GetFName();

    for (TObjectIterator<UClass> It; It; ++It)
    {
        if (It->IsChildOf(UNiagaraDataInterface::StaticClass()) && !It->HasAnyClassFlags(CLASS_Abstract))
        {
            UE_LOG(LogTemp, Warning, TEXT("Found DI: %s"), *It->GetName());
        }
    }
}
#endif