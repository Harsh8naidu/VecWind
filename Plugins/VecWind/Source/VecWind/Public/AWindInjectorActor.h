#pragma once
#include "CoreMinimal.h"  
#include "GameFramework/Actor.h"  
#include "WindVectorField.h"  

#include "AWindInjectorActor.generated.h"
UCLASS()  
class VECWIND_API AWindInjectorActor : public AActor  
{  
    GENERATED_BODY()  

public:  
    AWindInjectorActor();  

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wind Injector")  
    FVector VelocityToInject = FVector(0, 0, 1000);  

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wind Injector")  
    float Radius = 1.0f;  

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wind Injector")  
    bool bEnableInjection = true;  

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wind Injector")  
    TObjectPtr<class UWindVectorField> WindField;  

    UFUNCTION(BlueprintCallable, Category = "Debug")
    void ShowDebugSphere(bool persistentSphere, float lifetime);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug")
    bool bShowDebugSphereInPlayMode = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug")
    bool bShowDebugSphereAlwaysInEditor = false;

protected:  
    virtual void Tick(float DeltaTime) override;  
    virtual void BeginPlay() override;  
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual void OnConstruction(const FTransform& Transform);
    void DrawTemporaryDebugSphere();
    bool IsInEditorMode() const;
    
#if WITH_EDITOR
    virtual void PostEditMove(bool bFinished);
    virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent);
    virtual bool ShouldTickIfViewportsOnly() const override;
#endif

private:
    FVector InjectorLocation = FVector::ZeroVector;
    float TimeSinceLastInjection = 0.0f;
    
    // Controls how often we inject wind (in seconds)
    UPROPERTY(EditAnywhere, Category = "Wind|Performance")
    float InjectionInterval = 0.2f;
};