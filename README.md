# VecWind Getting Started

VecWind is a runtime wind vector field plugin for Unreal Engine 5 and Niagara. Create configurable 3D wind fields, sample wind velocity in CPU or GPU Niagara simulations, and drive dynamic effects such as snow, rain, embers, smoke, and debris.

VecWind includes adjustable grid size, cell size, wind bias, turbulence, noise settings, and wind strength. It also provides a Wind Injector Actor for adding localized velocity at runtime, plus demo content covering CPU simulation, GPU simulation, multi-field sampling, and wind injection.

Designed for artists and developers who want direct, flexible control over wind effects without modifying engine source.

## Requirements

- Unreal Engine 5.5
- Niagara enabled
- A C++ project, or a project able to build/load the VecWind plugin

VecWind was tested with Unreal Engine 5.5.4 on Windows.

## Installation

1. Copy the `VecWind` folder into your Unreal project:

YourProject/Plugins/VecWind

2. Open the project in Unreal Engine.

3. If prompted, rebuild the plugin modules.

4. Open Edit > Plugins, search for 'VecWind' and ensure it is enabled.

5. Restart the editor if Unreal requests it.

## Open the Demo Map

The included demo map is located at:

VecWind Content/Maps/VecWind_Demo_Level.umap

The map contains 4 Niagara demonstrations:
- CPU wind sampling
- GPU wind sampling
- GPU sampling with two WindField data interfaces in one Niagara system
- [add wind injector]

Use the in-level controls to activate one demonstration at a time.

## Add WindField to a Niagara System

1. Create or open a Niagara System. (Recommended Emitter: Hanging Particulates)
2. In the Niagara editor, add a User Parameter of type WindField.
    Ex: Go to the User Parameter section (Bottom-left section) -> Click + -> Expand the 'Make New' dropdown -> Expand the 'Data Interface' dropdown -> Scroll all the way to the bottom -> Select WindField
3. Once the WindField data interface is created, assign the UWindVectorField object using the dropdown listed in front of the Wind Field variable.
4. Configure embedded wind settings in the Details panel.
4. Create or open a Scratch Pad module in the particle update stage.
5. Read Particles.Position.
6. Use the WindField function:
   Sample Wind At Location
7. Connect the particle world position to the function's X, Y, and Z inputs.
8. Combine the returned OutX, OutY, and OutZ values into a vector.
9. Apply that vector to your particle velocity or acceleration.

A typical wind module works in the following way:

1. Sample WindField at current Particles.Position
2. Set Particles.Velocity = sampled wind velocity
3. Calculate DeltaMove = Particles.Velocity × Engine.DeltaTime
4. Calculate NewPosition = Particles.Position + DeltaMove
5. Set Particles.Position = NewPosition

Warning: The WindField sample returns a velocity vector. Do not overwrite Particles.Position with the sampled result.

## Wind Field Settings

The WindField data interface exposes these main settings:

Setting                                     Purpose         

1. Field Origin                             World-space origin of the wind grid. This does not control Nigara System spawn location.

2. Size X, Size Y, Size Z                   Number of grid cells along each axis.

3. Cell Size                                World-space size of one grid cell.

4. Wind Bias                                Base wind direction/bias.

5. Wind Scale                               Overall wind-strength multiplier.

6. Wind Noise Frequency                     Frequency of the generated wind noise.

7. Wind Noise Seed                          Seed used for repeatable noise.

8. Turbulence Strength                      Amount of noise-based variation in the wind.

9. Noise Scale                              Scale of the turbulence pattern. Lower values create broader features.

The field occupies approximately: (Size X × Cell Size) × (Size Y × Cell Size) × (Size Z × Cell Size)

Position your 'Field Origin' so the particles you want to affect are inside that world-space volume. Sampling outside the volume uses the nearest edge of the grid.

## Runtime Wind Injection

VecWind includes WindInjectorActor for runtime wind injection.

1. Place a WindInjectorActor in your level.
2. Assign the same Wind Field used by your Niagara Data Interface.
3. Configure:
   - Velocity To Inject
   - Radius
   - Enable Injection
   - Injection Interval
4. Move the actor in the level to change the injection location.

The actor updates the field origin to its own world position and injects wind at that position.

## CPU and GPU Usage

## CPU Niagara Simulation

Use CPU simulation when you need CPU-side Niagara behavior or a relatively modest number of particles.

## GPU Niagara Simulation

Use GPU simulation for denser weather effects and higher particle counts. The WindField Data Interface supports GPU HLSL generation and GPU buffer sampling.

## Multiple WindField Instances

A GPU Niagara System can contain multiple WindField data interfaces.

For example:

- WindField A pushes particles along X.
- WindField B pushes particles along Y.
- Applying both samples produces diagonal motion.

Each Niagara Data Interface instance generates and uses its own shader parameter symboll, allowing multiple WindField instances to coexist safely in one GPU System.

## Performance Notes

- Use CPU simulation for low-to-moderate particle counts.
- Prefer GPU simulation for dense weather effects.
- Particle lifetime, renderer/material cost, collision (if particles colliding with the surface as they are doing in the 1st demo), scene complexity, resolution, and hardware affect performance.
- The demo map activates one system at a time to keep performance representative.

Testing was performed with Unreal Engine 5.5.4 on Windows using a Ryzen 9 processor and RTX 4080 Laptop GPU.

# Troubleshooting

WindField does not appear in Niagara

- Confirm the VecWind plugin is enabled.
- Confirm Niagara is enabled.
- Restart Unreal after enabling or rebuilding the plugin.

## Particles do not respond to wind

- Confirm the Niagara system contains a WindField User Parameter.
- Confirm the Scratch Pad module calls 'Sample Wind At Location'
- Confirm the sampled vector is applied to velocity or acceleration.
- Confirm the particles are inside the configured field volume.
- Confirm the WindField grid dimension and Cell Size are greater than zero.

## GPU system does not compile

- Confirm the Niagara emitter uses GPU Compute Sim.
- Confirm each WindField parameter is connected to the intended sample node.
- Check the Output Log for Niagara shader compilation.

## Third-Party Notices

VecWind includes FastNoiseLite under the MIT Liecense. See:

../THIRD_PARTY_NOTICES.md
../Source/ThirdParty/FastNoise/LICENSE
