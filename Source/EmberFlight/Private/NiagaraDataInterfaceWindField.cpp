// Fill out your copyright notice in the Description page of Project Settings.


#include "NiagaraDataInterfaceWindField.h"
#include "NiagaraTypes.h"
#include "VectorVM.h"
#include "NiagaraShaderParametersBuilder.h"
#include "NiagaraCompileHashVisitor.h"
#include "RenderGraphBuilder.h"
#include <UnifiedBuffer.h>
#include "RenderGraphUtils.h"
#include "RenderGraphResources.h"
#include "NiagaraSystemInstance.h"
#include "RHIResources.h"
#include "RHI.h"
#include "NiagaraRenderer.h"

#define LOCTEXT_NAMESPACE "NiagaraWindFieldDI"

static const FName SampleWindFieldName(TEXT("SampleWindAtLocation"));
static const TCHAR* TemplateShaderFilePath = TEXT("/Plugin/Experimental/ChaosNiagara/NiagaraDataInterfaceWindField.ush");

UNiagaraDataInterfaceWindField::UNiagaraDataInterfaceWindField()
{
}

void UNiagaraDataInterfaceWindField::SampleWindAtLocation(FVectorVMExternalFunctionContext& Context)
{
    //UE_LOG(LogTemp, Warning, TEXT("Sample Wind At Locations called"));
    VectorVM::FUserPtrHandler<FNDIWindFieldInstanceData> InstanceData(Context);
    if (!InstanceData.Get() || !InstanceData.Get()->WindField)
    {
        UE_LOG(LogTemp, Warning, TEXT("UNiagaraDataInterfaceWindField::SampleWindAtLocation called with invalid FieldHandler."));
        return;
    }
    
    FNDIInputParam<float> X(Context);
    FNDIInputParam<float> Y(Context);
    FNDIInputParam<float> Z(Context);

    FNDIOutputParam<float> OutX(Context);
    FNDIOutputParam<float> OutY(Context);
    FNDIOutputParam<float> OutZ(Context);

    const int32 NumInstances = Context.GetNumInstances();

    for (int32 i = 0; i < NumInstances; ++i)
    {
        FVector WorldPos(X.GetAndAdvance(), Y.GetAndAdvance(), Z.GetAndAdvance());
        FVector Velocity = InstanceData.Get()->WindField->SampleWindAtPosition(WorldPos);

        //UE_LOG(LogTemp, Warning, TEXT("[Niagara] WindSample Velocity: X=%f Y=%f Z=%f"), Velocity.X, Velocity.Y, Velocity.Z);

        OutX.SetAndAdvance(Velocity.X);
        OutY.SetAndAdvance(Velocity.Y);
        OutZ.SetAndAdvance(Velocity.Z);
    }
}

void UNiagaraDataInterfaceWindField::GetFunctions(TArray<FNiagaraFunctionSignature>& OutFunctions)
{
    FNiagaraFunctionSignature Sig;
    Sig.Name = SampleWindFieldName;

    // Add the inputs of the data interface class
    Sig.Inputs.Add(FNiagaraVariable(FNiagaraTypeDefinition(GetClass()), TEXT("WindField")));

    // Add float position inputs
    Sig.Inputs.Add(FNiagaraVariable(FNiagaraTypeDefinition::GetFloatDef(), TEXT("X")));
    Sig.Inputs.Add(FNiagaraVariable(FNiagaraTypeDefinition::GetFloatDef(), TEXT("Y")));
    Sig.Inputs.Add(FNiagaraVariable(FNiagaraTypeDefinition::GetFloatDef(), TEXT("Z")));

    // Add float outputs
    Sig.Outputs.Add(FNiagaraVariable(FNiagaraTypeDefinition::GetFloatDef(), TEXT("OutX")));
    Sig.Outputs.Add(FNiagaraVariable(FNiagaraTypeDefinition::GetFloatDef(), TEXT("OutY")));
    Sig.Outputs.Add(FNiagaraVariable(FNiagaraTypeDefinition::GetFloatDef(), TEXT("OutZ")));

    Sig.bMemberFunction = true;
    Sig.bRequiresContext = false;
    Sig.bSupportsCPU = true;
    Sig.bSupportsGPU = true;

    Sig.SetDescription(LOCTEXT("SampleWindDesc", "Sample wind velocity at a given world position"));
    
    OutFunctions.Add(Sig);
}

DEFINE_NDI_DIRECT_FUNC_BINDER(UNiagaraDataInterfaceWindField, SampleWindAtLocation);

void UNiagaraDataInterfaceWindField::GetVMExternalFunction(const FVMExternalFunctionBindingInfo& BindingInfo, void* InstanceData, FVMExternalFunction& OutFunc)
{
    if (BindingInfo.Name == SampleWindFieldName)
    {
        NDI_FUNC_BINDER(UNiagaraDataInterfaceWindField, SampleWindAtLocation)::Bind(this, OutFunc);
    }
}

bool UNiagaraDataInterfaceWindField::CopyToInternal(UNiagaraDataInterface* Destination) const
{
    UNiagaraDataInterfaceWindField* DestTyped = Cast<UNiagaraDataInterfaceWindField>(Destination);
    if (!DestTyped)
    {
        return false;
    }

    DestTyped->WindField = this->WindField;

    return true;
}

bool UNiagaraDataInterfaceWindField::Equals(const UNiagaraDataInterface* Other) const
{
    const UNiagaraDataInterfaceWindField* OtherTyped = CastChecked<UNiagaraDataInterfaceWindField>(Other);
    return OtherTyped && OtherTyped->WindField == WindField;
}

bool UNiagaraDataInterfaceWindField::CanExecuteOnTarget(ENiagaraSimTarget Target) const
{
    return true; // Target == ENiagaraSimTarget::CPUSim/GPUSim (For CPU or GPU only)
}

void UNiagaraDataInterfaceWindField::PostInitProperties()
{
    Super::PostInitProperties();

    // Creating the Proxy here to ensure it's initialized both for CDO and non-CDO objects
    // It's required to pass data between the CPU and GPU (primarily for GPU based Niagara sytem)
    Proxy = MakeUnique<FNDIWindFieldProxy>();

    if (HasAnyFlags(RF_ClassDefaultObject))
    {
        ENiagaraTypeRegistryFlags Flags = ENiagaraTypeRegistryFlags::AllowAnyVariable | ENiagaraTypeRegistryFlags::AllowParameter;
        FNiagaraTypeRegistry::Register(FNiagaraTypeDefinition(GetClass()), Flags);
    }
    MarkRenderDataDirty();
}

bool UNiagaraDataInterfaceWindField::PerInstanceTick(void* PerInstanceData, FNiagaraSystemInstance* SystemInstance, float DeltaSeconds)
{
    FNDIWindFieldInstanceData* InstanceData = static_cast<FNDIWindFieldInstanceData*>(PerInstanceData);
    if (!InstanceData || !InstanceData->WindField || !InstanceData->InstanceDataOwner)
        return false;

    FNDIWindFieldData* DataOwner = InstanceData->InstanceDataOwner;

    // Only write into current write buffer
    int32 WriteIndex = DataOwner->WriteIndex;
    TArray<FVector4f>& WriteBuffer = DataOwner->VelocityGridBuffers[WriteIndex];

    const TArray<FVector>& SourceGrid = InstanceData->WindField->GetVelocityGrid();
    WriteBuffer.Reset(SourceGrid.Num());

    for (const FVector& V : SourceGrid)
    {
        WriteBuffer.Add(FVector4f(V.X, V.Y, V.Z, 0.0f));
    }

    return true; // request RT update
}

bool UNiagaraDataInterfaceWindField::PerInstanceTickPostSimulate(void* PerInstanceData, FNiagaraSystemInstance* InSystemInstance, float DeltaSeconds)
{
    FNDIWindFieldInstanceData* InstanceData = static_cast<FNDIWindFieldInstanceData*>(PerInstanceData);
    if (!InstanceData || !InstanceData->InstanceDataOwner)
        return false;

    // Swap buffers after sim
    FNDIWindFieldData* DataOwner = InstanceData->InstanceDataOwner;
    DataOwner->WriteIndex = 1 - DataOwner->WriteIndex;

    return false;
}

int32 UNiagaraDataInterfaceWindField::PerInstanceDataSize() const
{
    return sizeof(FNDIWindFieldInstanceData);
}

bool UNiagaraDataInterfaceWindField::InitPerInstanceData(void* PerInstanceData, FNiagaraSystemInstance* SystemInstance)
{
    FNDIWindFieldInstanceData* InstanceData = new (PerInstanceData) FNDIWindFieldInstanceData();
    InstanceData->WindField = WindField;

    if (!WindField || !SystemInstance)
    {
        return false;
    }

    // Create per-instance owner data
    FNDIWindFieldData* DataOwner = new FNDIWindFieldData();
    DataOwner->WindField = WindField;
    InstanceData->InstanceDataOwner = DataOwner;

    // Set field origin to the Niagara system's world location
    FVector SystemPos = SystemInstance->GetAttachComponent()->GetComponentLocation();
    WindField->FieldOrigin = SystemPos;
    /*UE_LOG(LogTemp, Warning, TEXT("[WindField] FieldOrigin set to %s for System %s"),
        *SystemPos.ToString(), *SystemInstance->GetSystem()->GetName());*/

    // Initialize CPU velocity grids
    const TArray<FVector>& SourceGrid = WindField->GetVelocityGrid();
    for (int32 i = 0; i < 2; ++i)
    {
        DataOwner->VelocityGridBuffers[i].Reset(); // clear first
        DataOwner->VelocityGridBuffers[i].Reserve(SourceGrid.Num());
        for (const FVector& V : SourceGrid)
        {
            DataOwner->VelocityGridBuffers[i].Add(FVector4f(V.X, V.Y, V.Z, 0.0f));
        }
    }
    InstanceData->WriteIndex = 0;
    DataOwner->WriteIndex = 0;

    // Initialize GPU buffer
    DataOwner->InitializeBufferIfNeeded(SourceGrid.Num());

    return true;
}

void UNiagaraDataInterfaceWindField::DestroyPerInstanceData(
    void* PerInstanceData,
    FNiagaraSystemInstance* SystemInstance)
{
    if (!PerInstanceData)
        return;

    FNDIWindFieldInstanceData* InstanceData =
        static_cast<FNDIWindFieldInstanceData*>(PerInstanceData);

    if (FNDIWindFieldData* DataOwner = InstanceData->InstanceDataOwner)
    {
        FNDIWindFieldProxy* WindFieldProxy = GetProxyAs<FNDIWindFieldProxy>();
        const FNiagaraSystemInstanceID InstanceID = SystemInstance->GetId();

        // Enqueued FIRST to guarantee that the proxy data is destroyed before the buffer is released
        ENQUEUE_RENDER_COMMAND(FWindFieldRemoveProxyEntry) (
            [WindFieldProxy, InstanceID](FRHICommandListImmediate& RHICmdList)
            {
                WindFieldProxy->DestroyPerInstanceData(InstanceID);
            }
        );

        DataOwner->ReleaseBuffer(); // enqueue release of GPU buffer on render thread
        InstanceData->InstanceDataOwner = nullptr; // avoid dangling pointer
        delete DataOwner; // frees CPU-side arrays on the game thread
    }

    InstanceData->~FNDIWindFieldInstanceData();
}

#if WITH_EDITORONLY_DATA
bool UNiagaraDataInterfaceWindField::AppendCompileHash(FNiagaraCompileHashVisitor* InVisitor) const
{
    bool bSuccess = Super::AppendCompileHash(InVisitor);

    bSuccess &= InVisitor->UpdateShaderFile(TemplateShaderFilePath);
    bSuccess &= InVisitor->UpdateShaderParameters<FNDIWindFieldShaderParameters>();

    return bSuccess;
}

void UNiagaraDataInterfaceWindField::GetParameterDefinitionHLSL(const FNiagaraDataInterfaceGPUParamInfo& ParamInfo, FString& OutHLSL)
{
    const TMap<FString, FStringFormatArg> TemplateArgs = {
        {TEXT("ParameterName"), ParamInfo.DataInterfaceHLSLSymbol} // This will be replaced at compile time by the stuff in .ush
    };
    AppendTemplateHLSL(OutHLSL, TemplateShaderFilePath, TemplateArgs);
}

bool UNiagaraDataInterfaceWindField::GetFunctionHLSL(const FNiagaraDataInterfaceGPUParamInfo& ParamInfo, const FNiagaraDataInterfaceGeneratedFunction& FunctionInfo, int FunctionInstanceIndex, FString& OutHLSL)
{
    if (Super::GetFunctionHLSL(ParamInfo, FunctionInfo, FunctionInstanceIndex, OutHLSL))
    {
        return true;
    }

    if (FunctionInfo.DefinitionName == SampleWindFieldName)
    {
        static const TCHAR* HlslTemplate = TEXT(R"(
            void {FunctionName}(float X, float Y, float Z, out float OutX, out float OutY, out float OutZ)
            {
                float3 WorldPos = float3(X, Y, Z);
                float3 WindVelocity = {ParameterName}_SampleWindTexture(WorldPos);
                OutX = WindVelocity.x;
                OutY = WindVelocity.y;
                OutZ = WindVelocity.z;
            }
            )");

        TMap<FString, FStringFormatArg> Args;
        Args.Add(TEXT("FunctionName"), FunctionInfo.InstanceName);
        Args.Add(TEXT("ParameterName"), ParamInfo.DataInterfaceHLSLSymbol);

        OutHLSL += FString::Format(HlslTemplate, Args);
        return true;
    }

    return false;
}

#endif

void UNiagaraDataInterfaceWindField::BuildShaderParameters(FNiagaraShaderParametersBuilder& ShaderParametersBuilder) const
{
    ShaderParametersBuilder.AddNestedStruct<FNDIWindFieldShaderParameters>();
}

void UNiagaraDataInterfaceWindField::SetShaderParameters(
    const FNiagaraDataInterfaceSetShaderParametersContext& Context) const
{
    const FNDIWindFieldProxy& DIProxy = Context.GetProxy<FNDIWindFieldProxy>();
    const FNDIWindFieldRenderData* RenderData = DIProxy.SystemInstancesToProxyData.Find(Context.GetSystemInstanceID());

    auto* ShaderParameters = Context.GetParameterNestedStruct<FNDIWindFieldShaderParameters>();
    if (!RenderData || !ShaderParameters)
        return;

    // Basic field properties
    ShaderParameters->FieldOrigin = RenderData->FieldOrigin;
    ShaderParameters->CellSize = RenderData->CellSize;
    ShaderParameters->SizeX = RenderData->SizeX;
    ShaderParameters->SizeY = RenderData->SizeY;
    ShaderParameters->SizeZ = RenderData->SizeZ;

    // Bind the SRV from our GPU buffer
    FNDIWindFieldBuffer* Buffer = RenderData->AssetBuffer;
    ShaderParameters->VelocityGridSRV =
        FNiagaraRenderer::GetSrvOrDefaultFloat4(Buffer ? Buffer->VelocityGridSRV : nullptr);
}

void UNiagaraDataInterfaceWindField::ProvidePerInstanceDataForRenderThread(
    void* DataForRenderThread,
    void* PerInstanceData,
    const FNiagaraSystemInstanceID& SystemInstance)
{
    FNDIWindFieldInstanceData* InstanceData = static_cast<FNDIWindFieldInstanceData*>(PerInstanceData);
    FNDIWindFieldRenderData* RenderData = new (DataForRenderThread) FNDIWindFieldRenderData();

    if (!InstanceData || !InstanceData->InstanceDataOwner)
    {
        UE_LOG(LogTemp, Warning, TEXT("[WindField] ProvidePerInstanceData skipped: InstanceData or DataOwner null"));
        return;
    }

    FNDIWindFieldData* DataOwner = InstanceData->InstanceDataOwner;

    // --- Select the read buffer (double buffering) ---
    const int32 ReadIndex = 1 - DataOwner->WriteIndex;
    const TArray<FVector4f>& ReadBuffer = DataOwner->VelocityGridBuffers[ReadIndex];

    // Copy Velocity grid data from the read buffer to the render data
    RenderData->VelocityGridData = ReadBuffer;

    // --- Copy basic field info from the asset ---
    if (UWindVectorField* Field = DataOwner->WindField)
    {
        RenderData->FieldOrigin = FVector3f(Field->FieldOrigin);
        RenderData->CellSize = Field->CellSize;
        RenderData->SizeX = Field->SizeX;
        RenderData->SizeY = Field->SizeY;
        RenderData->SizeZ = Field->SizeZ;
    }

    // --- AssetBuffer is the raw pointer of the shared buffer ---
    RenderData->AssetBuffer = DataOwner->AssetBuffer.Get();

    // --- Mark that this frame has new data ---
    RenderData->bUploadQueuedThisFrame = true;

    /*UE_LOG(LogTemp, Warning, TEXT("[WindField] ProvidePerInstanceData: Prepared %d elements for instance %llu"),
        RenderData->VelocityGridCount, SystemInstance);*/
}

void FNDIWindFieldProxy::ConsumePerInstanceDataFromGameThread(void* PerInstanceData, const FNiagaraSystemInstanceID& Instance)
{
    check(IsInRenderingThread());

    FNDIWindFieldRenderData* SourceData = static_cast<FNDIWindFieldRenderData*>(PerInstanceData);
    if (!SourceData)
    {
        UE_LOG(LogTemp, Warning, TEXT("[WindField] ConsumePerInstanceData: SourceData null for Instance %llu"), Instance);
        return;
    }

    FNDIWindFieldRenderData& TargetData = SystemInstancesToProxyData.FindOrAdd(Instance);

    // Copy POD fields
    TargetData.FieldOrigin = SourceData->FieldOrigin;
    TargetData.CellSize = SourceData->CellSize;
    TargetData.SizeX = SourceData->SizeX;
    TargetData.SizeY = SourceData->SizeY;
    TargetData.SizeZ = SourceData->SizeZ;
    TargetData.AssetBuffer = SourceData->AssetBuffer;
    TargetData.bUploadQueuedThisFrame = SourceData->bUploadQueuedThisFrame;

    // Since this is a render thread
    // SourceData is transient (about to be destroyed once consumed), so we move instead of copying
    TargetData.VelocityGridData = MoveTemp(SourceData->VelocityGridData);
}

void FNDIWindFieldProxy::PreStage(const FNDIGpuComputePreStageContext& Context)
{
    check(IsInRenderingThread());

    FNDIWindFieldRenderData* RenderData = SystemInstancesToProxyData.Find(Context.GetSystemInstanceID());
    if (!RenderData || !RenderData->AssetBuffer)
        return;

    FNDIWindFieldBuffer* Buffer = RenderData->AssetBuffer;
    if (!Buffer->VelocityGridBufferRHI.IsValid())
        return; // Avoid crash if InitRHI not done yet

    const int32 NumElements = RenderData->VelocityGridData.Num();
    if (NumElements == 0)
        return;

    Buffer->NumElements = NumElements;

    FRHICommandListImmediate& RHICmdList = FRHICommandListExecutor::GetImmediateCommandList();

    // Direct memcpy from GT double buffer to GPU buffer
    void* Dest = RHICmdList.LockBuffer(
        Buffer->VelocityGridBufferRHI,
        0,
        NumElements * sizeof(FVector4f),
        RLM_WriteOnly
    );
    FMemory::Memcpy(Dest, RenderData->VelocityGridData.GetData(), NumElements * sizeof(FVector4f));
    RHICmdList.UnlockBuffer(Buffer->VelocityGridBufferRHI);

    RenderData->bUploadQueuedThisFrame = false;
}

void FNDIWindFieldProxy::InitializePerInstanceData(const FNiagaraSystemInstanceID& SystemInstance)
{
    check(IsInRenderingThread());
    check(!SystemInstancesToProxyData.Contains(SystemInstance));

    SystemInstancesToProxyData.Add(SystemInstance);
}

void FNDIWindFieldProxy::DestroyPerInstanceData(const FNiagaraSystemInstanceID& SystemInstance)
{
    check(IsInRenderingThread());

    SystemInstancesToProxyData.Remove(SystemInstance);
}

void FNDIWindFieldData::InitializeBufferIfNeeded(int32 NumElements)
{
    if (AssetBuffer.IsValid())
    {
        // If already initialized with the same or larger size, skip
        if (AssetBuffer->NumElements >= NumElements && AssetBuffer->bIsInitialized)
        {
            return;
        }

        // Otherwise, release old buffer first
        ReleaseBuffer();
    }

    // Create new buffer
    AssetBuffer = MakeShared<FNDIWindFieldBuffer, ESPMode::ThreadSafe>();
    AssetBuffer->NumElements = NumElements;

    // This schedules InitRHI on render thread
    BeginInitResource(AssetBuffer.Get());

    //UE_LOG(LogTemp, Warning, TEXT("[WindFieldData] Created AssetBuffer with %d elements, BeginInitResource called."), NumElements);
}

void FNDIWindFieldData::ReleaseBuffer()
{
    if (AssetBuffer.IsValid())
    {
        // Release on render thread safely
        TSharedPtr<FNDIWindFieldBuffer> BufferToRelease = AssetBuffer;
        ENQUEUE_RENDER_COMMAND(ReleaseWindFieldBuffer)(
            [BufferToRelease](FRHICommandListImmediate& RHICmdList)
            {
                BufferToRelease->ReleaseResource();
            });

        AssetBuffer.Reset();
        //UE_LOG(LogTemp, Warning, TEXT("[WindFieldData] AssetBuffer released."));
    }
}

void FNDIWindFieldBuffer::InitRHI(FRHICommandListBase& RHICmdList)
{
    // Prevent double initialization
    if (bIsInitialized)
    {
        UE_LOG(LogTemp, Warning, TEXT("[WindField::InitRHI] Already initialized, skipping."));
        return;
    }

    // Release any stale resources first
    VelocityGridBufferRHI.SafeRelease();
    VelocityGridSRV.SafeRelease();

    if (NumElements <= 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("[WindField::InitRHI] NumElements is 0. No buffer will be created."));
        return;
    }

    const uint32 Stride = sizeof(FVector4f);
    const uint32 BufferSize = NumElements * Stride;

    FRHIResourceCreateInfo CreateInfo(TEXT("WindField.VelocityGrid"));
    VelocityGridBufferRHI = RHICmdList.CreateStructuredBuffer(Stride, BufferSize, BUF_ShaderResource | BUF_StructuredBuffer | BUF_Static, CreateInfo);

    if (!VelocityGridBufferRHI.IsValid())
    {
        UE_LOG(LogTemp, Error, TEXT("[WindField::InitRHI] Failed to create structured buffer!"));
        return;
    }

    // Create SRV for the buffer
    auto SRVDesc = FRHIViewDesc::CreateBufferSRV();
    SRVDesc.SetType(FRHIViewDesc::EBufferType::Structured);
    SRVDesc.SetStride(Stride);

    VelocityGridSRV = RHICmdList.CreateShaderResourceView(VelocityGridBufferRHI, SRVDesc);

    if (!VelocityGridSRV.IsValid())
    {
        UE_LOG(LogTemp, Error, TEXT("[WindField::InitRHI] Failed to create SRV for buffer!"));
        VelocityGridBufferRHI.SafeRelease();
        return;
    }

    bIsInitialized = true;

    /*UE_LOG(LogTemp, Warning,
        TEXT("[WindField::InitRHI] SRV created. NumElements=%d, RHI=%p, SRV=%p"),
        NumElements,
        VelocityGridBufferRHI.GetReference(),
        VelocityGridSRV.GetReference()
    );*/
}

void FNDIWindFieldBuffer::ReleaseRHI()
{
    if (VelocityGridBufferRHI.IsValid() || VelocityGridSRV.IsValid())
    {
        UE_LOG(LogTemp, Warning,
            TEXT("[WindField::ReleaseRHI] Releasing GPU resources. Buffer=%p SRV=%p"),
            VelocityGridBufferRHI.GetReference(),
            VelocityGridSRV.GetReference());
    }

    VelocityGridBufferRHI.SafeRelease();
    VelocityGridSRV.SafeRelease();
    bIsInitialized = false;
}

#if WITH_EDITOR
void UNiagaraDataInterfaceWindField::PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent)
{
    Super::PostEditChangeProperty(PropertyChangedEvent);
    MarkRenderDataDirty();
}
#endif //WITH_EDITOR
#undef LOCTEXT_NAMESPACE