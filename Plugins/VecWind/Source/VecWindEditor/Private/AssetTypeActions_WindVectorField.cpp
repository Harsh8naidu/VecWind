// Fill out your copyright notice in the Description page of Project Settings.

#include "AssetTypeActions_WindVectorField.h"
#include "WindVectorField.h"

UClass* FAssetTypeActions_WindVectorField::GetSupportedClass() const
{
    return UWindVectorField::StaticClass();
}