// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AssetTypeActions_Base.h"

class FAssetTypeActions_WindVectorField : public FAssetTypeActions_Base
{
public:
    virtual FText GetName() const override { return NSLOCTEXT("AssetTypeActions", "FWindVectorFieldAsset", "Wind Vector Field"); }

    virtual FColor GetTypeColor() const override { return FColor::Cyan; }

    virtual UClass* GetSupportedClass() const override;

    virtual uint32 GetCategories() override { return EAssetTypeCategories::Misc; }
};