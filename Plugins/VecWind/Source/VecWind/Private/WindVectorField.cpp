// Fill out your copyright notice in the Description page of Project Settings.

#include "WindVectorField.h"
#include "EngineUtils.h"

UWindVectorField::UWindVectorField() 
{
    
}

void UWindVectorField::Initialize()
{
    if (bInitialized || SizeX <= 0 || SizeY <= 0 || SizeZ <= 0 || CellSize <= 0.0f)
    {
        return;
    }

    VelocityGrid.SetNumZeroed(SizeX * SizeY * SizeZ);

    Noise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
    Noise.SetFrequency(WindNoiseFrequency);
    Noise.SetSeed(WindNoiseSeed);

    // Warmup wind field
    const int WarmUpFrames = 10;
    const float FixedDeltaTime = 0.016f;
    for (int i = 0; i < WarmUpFrames; ++i)
    {
        Update(FixedDeltaTime);
    }

    bInitialized = true;
}

void UWindVectorField::PostLoad()
{
    Super::PostLoad();

    if (VelocityGrid.Num() == 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("PostLoad: Initializing wind field"));
        Initialize();
    }
}

int UWindVectorField::GetIndex(int X, int Y, int Z) const
{
    return X + Y * SizeX + Z * SizeX * SizeY;
}

bool UWindVectorField::IsValidIndex(int X, int Y, int Z) const
{
    return X >= 0 && X < SizeX &&
           Y >= 0 && Y < SizeY &&
           Z >= 0 && Z < SizeZ;
}

void UWindVectorField::Advect(float DeltaTime)
{
    // Temporary array to hold new velocities after advection
    TArray<FVector> NewVelocityGrid;
    NewVelocityGrid.SetNumZeroed(VelocityGrid.Num());

    for (int z = 0; z < SizeZ; ++z)
    {
        for (int y = 0; y < SizeY; ++y)
        {
            for (int x = 0; x < SizeX; ++x)
            {
                int idx = GetIndex(x, y, z);
                FVector currentVelocity = VelocityGrid[idx];

                // Calculate where the wind came from (backtrace)
                FVector worldPos = FVector(x, y, z) * CellSize;
                FVector prevPos = worldPos - currentVelocity * DeltaTime;

                // Convert prevPos to grid coords (from world coordinates)
                FVector gridPos = prevPos / CellSize;

                // Trilinear interpolation for velocity at prevPos
                FVector advectedVelocity = SampleVelocityAtGridPosition(gridPos);

                NewVelocityGrid[idx] = advectedVelocity;
            }
        }
    }

    VelocityGrid = NewVelocityGrid;
}

void UWindVectorField::DecayVelocity(float DeltaTime)
{
    float decayRate = 1.0f; // Adjust this to control how fast wind slows down

    for (FVector& vel : VelocityGrid)
    {
        vel *= FMath::Max(0.0f, 1.0f - decayRate * DeltaTime);
    }
}

FVector const UWindVectorField::SampleVelocityAtGridPosition(const FVector& GridPos) const
{
    // Ensure grid in initialized
    if (VelocityGrid.Num() == 0 || SizeX <= 1 || SizeY <= 1 || SizeZ <= 1)
    {
        UE_LOG(LogTemp, Warning, TEXT("SampleVelocityAtGridPosition called with uninitialized or too small grid"));
        return FVector::ZeroVector;
    }

    // GridPos components can be fractional
    int x0 = FMath::FloorToInt(GridPos.X);
    int y0 = FMath::FloorToInt(GridPos.Y);
    int z0 = FMath::FloorToInt(GridPos.Z);

    int x1 = x0 + 1;
    int y1 = y0 + 1;
    int z1 = z0 + 1;

    // Clamp indices to enure they're within bounds
    x0 = FMath::Clamp(x0, 0, SizeX - 1);
    y0 = FMath::Clamp(y0, 0, SizeY - 1);
    z0 = FMath::Clamp(z0, 0, SizeZ - 1);

    x1 = FMath::Clamp(x1, 0, SizeX - 1);
    y1 = FMath::Clamp(y1, 0, SizeY - 1);
    z1 = FMath::Clamp(z1, 0, SizeZ - 1);

    // Ensure all indices are still valid
    auto SafeGet = [&](int X, int Y, int Z)
    {
        return IsValidIndex(X, Y, Z) ? VelocityGrid[GetIndex(X, Y, Z)] : FVector::ZeroVector;
    };

    // Get Velocity at corners
    FVector c000 = SafeGet(x0, y0, z0);
    FVector c100 = SafeGet(x1, y0, z0);
    FVector c010 = SafeGet(x0, y1, z0);
    FVector c110 = SafeGet(x1, y1, z0);
    FVector c001 = SafeGet(x0, y0, z1);
    FVector c101 = SafeGet(x1, y0, z1);
    FVector c011 = SafeGet(x0, y1, z1);
    FVector c111 = SafeGet(x1, y1, z1);

    // Fractional distance within the cell
    float sx = GridPos.X - x0;
    float sy = GridPos.Y - y0;
    float sz = GridPos.Z - z0;

    // Interpolate along X
    FVector c00 = FMath::Lerp(c000, c100, sx);
    FVector c10 = FMath::Lerp(c010, c110, sx);
    FVector c01 = FMath::Lerp(c001, c101, sx);
    FVector c11 = FMath::Lerp(c011, c111, sx);

    // Interpolate along Y
    FVector c0 = FMath::Lerp(c00, c10, sy);
    FVector c1 = FMath::Lerp(c01, c11, sy);

    // Interpolate along Z
    FVector c = FMath::Lerp(c0, c1, sz);

    //UE_LOG(LogTemp, Warning, TEXT("SampleWindAtPosition called on initialized field. Asset name: %s"), *GetNameSafe(this));

    return c;
}

void UWindVectorField::Update(float DeltaTime)
{
    if (VelocityGrid.Num() == 0)
    {
        UE_LOG(LogTemp, Error, TEXT("[WindField] Update called before Initialize! Skipping update."));
        return;
    }

    Advect(DeltaTime);
    DecayVelocity(DeltaTime);

    //float Time = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;

    for (int Z = 0; Z < SizeZ; ++Z)
    {
        for (int Y = 0; Y < SizeY; ++Y)
        {
            for (int X = 0; X < SizeX; ++X)
            {
                int Index = GetIndex(X, Y, Z);
                FVector PosWorld = FVector(X, Y, Z) * CellSize;

                // Sample noise for turbulence
                float TurbX = Noise.GetNoise((float)X * NoiseScale, (float)Y * NoiseScale, (float)Z * NoiseScale);
                float TurbY = Noise.GetNoise((float)X * NoiseScale + 1000, (float)Y * NoiseScale + 1000, (float)Z * NoiseScale + 1000);
                float TurbZ = Noise.GetNoise((float)X * NoiseScale + 2000, (float)Y * NoiseScale + 2000, (float)Z * NoiseScale + 2000);

                // Make turbulence gentle
                FVector Turbulence = FVector(TurbX, TurbY, TurbZ) * TurbulenceStrength;

                // Combine steady bias and turbulence
                FVector WindVelocity = (WindBias + Turbulence) * WindScale;

                // Apply to velocity grid
                VelocityGrid[Index] += WindVelocity * DeltaTime;
            }
        }
    }
}

void UWindVectorField::InjectWindAtPosition(const FVector& WorldPos, const FVector& VelocityToInject, float Radius)
{
    FVector LocalWorldPos = WorldPos - FieldOrigin;
    FVector GridPosF = LocalWorldPos / CellSize;

    // Calculate the affected grid cells within radius
    int minX = FMath::Clamp(FMath::FloorToInt(GridPosF.X - Radius / CellSize), 0, SizeX - 1);
    int maxX = FMath::Clamp(FMath::CeilToInt(GridPosF.X + Radius / CellSize), 0, SizeX - 1);
    int minY = FMath::Clamp(FMath::FloorToInt(GridPosF.Y - Radius / CellSize), 0, SizeY - 1);
    int maxY = FMath::Clamp(FMath::CeilToInt(GridPosF.Y + Radius / CellSize), 0, SizeY - 1);
    int minZ = FMath::Clamp(FMath::FloorToInt(GridPosF.Z - Radius / CellSize), 0, SizeZ - 1);
    int maxZ = FMath::Clamp(FMath::CeilToInt(GridPosF.Z + Radius / CellSize), 0, SizeZ - 1);
    
    for (int z = minZ; z <= maxZ; ++z)
    {
        for (int y = minY; y <= maxY; ++y) 
        {
            for (int x = minX; x <= maxX; ++x) 
            {
                FVector cellCenterLocal = FVector(x, y, z) * CellSize + FVector(CellSize * 0.5f);
                float dist = FVector::Dist(cellCenterLocal, LocalWorldPos);

                if (dist <= Radius && IsValidIndex(x, y, z))
                {
                    int idx = GetIndex(x, y, z);
                    // Add velocity scaled by how close cell is to center
                    float strength = 1.0f - (dist / Radius);
                    VelocityGrid[idx] += VelocityToInject * strength;
                }
            }
        }
    }
}

FVector UWindVectorField::SampleWindAtPosition(const FVector& WorldPos) const
{
    if (VelocityGrid.Num() == 0 || SizeX <= 1 || SizeY <= 1 || SizeZ <= 1)
    {
        UE_LOG(LogTemp, Warning, TEXT("SampleWindAtPosition called on uninitialized field. Asset name: %s"), *GetNameSafe(this));
        return FVector::ZeroVector;
    }

    FVector GridPos = WorldPos / CellSize;
    return SampleVelocityAtGridPosition(GridPos);
}

FVector UWindVectorField::GetPhoenixPosition() const
{
    UWorld* World = GetWorld();
    if (!World) return FVector::ZeroVector;

    // Get the BP_Character (Phoenix) by tag
    for (TActorIterator<AActor> It(World); It; ++It) {
        AActor* Actor = *It;
        if (Actor && Actor->ActorHasTag("Phoenix"))
        {
            return Actor->GetActorLocation();
        }
    }

    return FVector::ZeroVector;
}

void UWindVectorField::DebugDraw(float Scale) const
{
    UWorld* World = GetWorld(); // get world from the UObject base
    if (!World) return;

    FVector PhoenixPos = GetPhoenixPosition();
    UE_LOG(LogTemp, Warning, TEXT("Phoenix Position: %s"), *PhoenixPos.ToString());
    
    float MaxDrawDistance = 3000.0f;
    int Step = 3;
    FVector GridOrigin = PhoenixPos - FVector(SizeX, SizeY, SizeZ) * 0.5f * CellSize;

    for (int z = 0; z < SizeZ; z += Step)
    {
        for (int y = 0; y < SizeY; y += Step)
        {
            for (int x = 0; x < SizeX; x += Step)
            {
                int Index = GetIndex(x, y, z);
                FVector Start = GridOrigin + FVector(x, y, z) * CellSize;

                FVector Velocity = VelocityGrid[Index];
                if (Velocity.IsNearlyZero()) continue;

                FVector End = Start + (Velocity * Scale * 0.1f);

                DrawDebugDirectionalArrow(World, Start, End, 50.0, FColor::Blue, false, -0.5f, 0, 1.0f);
            }
        }
    }
}

void UWindVectorField::ResetField()
{
    VelocityGrid.Empty();
    VelocityGrid.SetNumZeroed(SizeX * SizeY * SizeZ);
    Initialize();
}

#if WITH_EDITOR
void UWindVectorField::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
    Super::PostEditChangeProperty(PropertyChangedEvent);

    bInitialized = false;

    ResetField();

    // Mark the asset as modified
    MarkPackageDirty();
}
#endif