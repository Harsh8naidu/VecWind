#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

class FVecWindModule final : public IModuleInterface
{
public:
    virtual void StartupModule() override;
};