// Fill out your copyright notice in the Description page of Project Settings.
#pragma once
#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "DrawDebugHelpers.h"
#include "FastNoiseLite.h"
#include "WindVectorField.generated.h"
UCLASS(Blueprintable, EditInlineNew, DefaultToInstanced)
class VECWIND_API UWindVectorField : public UObject
{
    GENERATED_BODY()

public:
	UWindVectorField();

    UFUNCTION(BlueprintCallable, Category="Wind Field")
    void Initialize();
    UFUNCTION(BlueprintCallable, Category="Wind Field")
    void Update(float DeltaTime);
    UFUNCTION(BlueprintCallable, Category = "Wind Field")
    void InjectWindAtPosition(const FVector& WorldPos, const FVector& VelocityToInject, float Radius);
    UFUNCTION(BlueprintCallable, Category="Wind Field")
    FVector SampleWindAtPosition(const FVector& WorldPos) const;
    UFUNCTION(BlueprintCallable, Category="Wind Field")
    void DebugDraw(float Scale = 100.0f) const;

    void ResetField();

    const TArray<FVector>& GetVelocityGrid() const { return VelocityGrid; }

    // ======= Editable Parameters =======

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wind Field|Grid")
    FVector FieldOrigin = FVector::ZeroVector;

    // Grid size
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wind Field|Grid")
    int32 SizeX = 30;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wind Field|Grid")
    int32 SizeY = 30;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wind Field|Grid")
    int32 SizeZ = 30;

    // World-space cell size
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wind Field|Grid")
    float CellSize = 100.0f;

    // Noise properties
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wind Field")
    float WindNoiseFrequency = 0.01f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wind Field")
    float WindNoiseSeed = 1337;

    /** Strength multiplier for wind velocity overall */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wind Field")
    float WindScale = 300.0f;

    /** Base wind direction (world space, normalized, can be set per field) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wind Field", meta = (ToolTip = "Base wind direction (world space, normalized, can be set per field)"))
    FVector WindBias = FVector(1.0f, 0.0f, 0.1f);
    
    /** Amount of noise-based turbulence applied to the wind */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wind Field", meta = (ToolTip = "Amount of noise-based turbulence applied to the wind"))
    float TurbulenceStrength = 0.2f;

    /** Scale for noise pattern – lower = wider noise waves */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wind Field", meta = (ToolTip = "Controls how spread out the noise features are (lower = larger features)."))
    float NoiseScale = 0.01f;

protected:
    virtual void PostLoad() override;
#if WITH_EDITOR
    virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

private:
    float MaxTurbulence = 20.0;
    bool bIncreasing = false;
    bool bInitialized = false;
    bool isDone = false;

    // Simulation grid
    TArray<FVector> VelocityGrid;

    // Noise generator
    FastNoiseLite Noise;
    
    // Helpers
    int GetIndex(int X, int Y, int Z) const;
    bool IsValidIndex(int X, int Y, int Z) const;
    void Advect(float DeltaTime);
    void DecayVelocity(float DeltaTime);
    FVector const SampleVelocityAtGridPosition(const FVector& GridPos) const;
    FVector GetPhoenixPosition() const;
};