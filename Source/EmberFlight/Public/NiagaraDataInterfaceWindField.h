// Fill out your copyright notice in the Description page of Project Settings.
#pragma once
#include "RenderResource.h"
#include "RenderGraphResources.h"
#include "CoreMinimal.h"
#include "NiagaraDataInterface.h"
#include "WindVectorField.h"
#include "NiagaraDataInterfaceWindField.generated.h"

UCLASS(EditInlineNew, Category = "Wind", meta = (DisplayName = "WindField", NiagaraDataInterface = "True"), Blueprintable, BlueprintType)
class EMBERFLIGHT_API UNiagaraDataInterfaceWindField : public UNiagaraDataInterface
{
    GENERATED_BODY()

public:
    UNiagaraDataInterfaceWindField();
    void SampleWindAtLocation(FVectorVMExternalFunctionContext& Context);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wind")
    TObjectPtr<UWindVectorField> WindField;

    // CPU Sim Functionality
    virtual void GetFunctions(TArray<FNiagaraFunctionSignature>& OutFunctions) override;
    virtual void GetVMExternalFunction(const FVMExternalFunctionBindingInfo& BindingInfo, void* InstanceData, FVMExternalFunction& OutFunc) override;
    virtual bool CopyToInternal(UNiagaraDataInterface* Destination) const override;
    virtual bool Equals(const UNiagaraDataInterface* Other) const override;
    virtual bool CanExecuteOnTarget(ENiagaraSimTarget Target) const override;
    virtual void PostInitProperties() override;
    virtual int32 PerInstanceDataSize() const override;
    virtual bool InitPerInstanceData(void* PerInstanceData, FNiagaraSystemInstance* SystemInstance) override;
    virtual void DestroyPerInstanceData(void* PerInstanceData, FNiagaraSystemInstance* SystemInstance) override;

    // GPU Sim Functionality
#if WITH_EDITORONLY_DATA
    virtual bool AppendCompileHash(FNiagaraCompileHashVisitor* InVisitor) const override;
    virtual void GetParameterDefinitionHLSL(const FNiagaraDataInterfaceGPUParamInfo& ParamInfo, FString& OutHLSL) override;
    virtual bool GetFunctionHLSL(const FNiagaraDataInterfaceGPUParamInfo& ParamInfo, const FNiagaraDataInterfaceGeneratedFunction& FunctionInfo, int FunctionInstanceIndex, FString& OutHLSL) override;
#endif
    virtual void BuildShaderParameters(FNiagaraShaderParametersBuilder& ShaderParametersBuilder) const override;
    virtual void SetShaderParameters(const FNiagaraDataInterfaceSetShaderParametersContext& Context) const override;
    virtual void ProvidePerInstanceDataForRenderThread(void* DataForRenderThread, void* PerInstanceData, const FNiagaraSystemInstanceID& SystemInstance) override;
    virtual bool PerInstanceTick(void* PerInstanceData, FNiagaraSystemInstance* SystemInstance, float DeltaSeconds) override;
    virtual bool PerInstanceTickPostSimulate(void* PerInstanceData, FNiagaraSystemInstance* InSystemInstance, float DeltaSeconds);
    virtual bool HasPreSimulateTick() const override { return true; }
    virtual bool HasPostSimulateTick() const override { return true; }
    virtual bool HasTickGroupPrereqs() const override { return true; }
    
#if WITH_EDITOR
    virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
};

struct FNDIWindFieldBuffer : public FRenderResource
{
    FBufferRHIRef VelocityGridBufferRHI; // Persistent GPU buffer
    FShaderResourceViewRHIRef VelocityGridSRV; // SRV for Niagara shader binding

    int32 NumElements = 0;
    bool bIsInitialized = false;

    virtual void InitRHI(FRHICommandListBase& RHICmdList) override;
    virtual void ReleaseRHI() override;
};

// GPU Shader Parameters Struct
BEGIN_SHADER_PARAMETER_STRUCT(FNDIWindFieldShaderParameters, )
    SHADER_PARAMETER(FVector3f, FieldOrigin)
    SHADER_PARAMETER(float, CellSize)
    SHADER_PARAMETER(uint32, SizeX)
    SHADER_PARAMETER(uint32, SizeY)
    SHADER_PARAMETER(uint32, SizeZ)
    SHADER_PARAMETER_SRV(StructuredBuffer<float4>, VelocityGridSRV)
END_SHADER_PARAMETER_STRUCT()

struct FNDIWindFieldData;

// Minimal per-instance data, just references and state, no ownership of buffer
USTRUCT()
struct FNDIWindFieldInstanceData
{
    GENERATED_BODY()

    UPROPERTY()
    UWindVectorField* WindField = nullptr;
    
    // Pointer to the shared per-instance data that owns the buffer and CPU grids
    FNDIWindFieldData* InstanceDataOwner = nullptr;

    int32 WriteIndex = 0;

    // Destructor to ensure breaking pointer links (not really needed tho but safe)
    void Reset()
    {
        WindField = nullptr;
        InstanceDataOwner = nullptr;
        WriteIndex = 0;
    }
};

struct FNDIWindFieldRenderData
{
    FVector3f FieldOrigin;
    float CellSize;
    int32 SizeX;
    int32 SizeY;
    int32 SizeZ;

    TArray<FVector4f> VelocityGridData; // render thread copy of the velocity grid data for this instance

    FNDIWindFieldBuffer* AssetBuffer = nullptr; // Raw pointer to GPU buffer, not owning - managed by FNDIWindFieldData
    bool bUploadQueuedThisFrame = false;
};

// This struct owns the CPU velocity arrays AND the GPU buffer resource
struct FNDIWindFieldData
{
    // WindField pointer, set once per instance init
    UPROPERTY()
    UWindVectorField* WindField = nullptr;

    // Double-buffered velocity grids stored on CPU
    TArray<FVector4f> VelocityGridBuffers[2];

    int32 WriteIndex = 0;

    // Shared GPU buffer resource used for rendering (owned here, shared with render thread)
    TSharedPtr<FNDIWindFieldBuffer, ESPMode::ThreadSafe> AssetBuffer;

    // Initialize and manage buffer lifecycle here (e.g. Init, Release functions)
    void InitializeBufferIfNeeded(int32 NumElements);
    void ReleaseBuffer();
};

struct FNDIWindFieldProxy : public FNiagaraDataInterfaceProxy
{
    /** List of proxy data for each system instances **/
    TMap<FNiagaraSystemInstanceID, FNDIWindFieldRenderData> SystemInstancesToProxyData;

    /** Initialize the Proxy data buffer */
    void InitializePerInstanceData(const FNiagaraSystemInstanceID& SystemInstance);

    /** Destroy the proxy data if necessary */
    void DestroyPerInstanceData(const FNiagaraSystemInstanceID& SystemInstance);

    /** Get the size of the data that will be passed to render **/
    virtual int32 PerInstanceDataPassedToRenderThreadSize() const override { return sizeof(FNDIWindFieldRenderData); }

    /** Get the data that will be passed to render **/
    virtual void ConsumePerInstanceDataFromGameThread(void* PerInstanceData, const FNiagaraSystemInstanceID& Instance) override;

    /** Launch all pre stage functions **/
    virtual void PreStage(const FNDIGpuComputePreStageContext& Context) override;
};