# VecWind Demo Map

The VecWind demo map provides four small examples of the plugin in use.

## Opening the map

1. Enable the **VecWind** plugin.
2. Open `VecWind_Demo_Level` from the plugin Content folder.
3. Press Play in Editor.

## Demo controls

Key   Demo

`1` - CPU Niagara wind sampling
`2` - GPU Niagara wind sampling
`3` - GPU Niagara system using two WindField Data Interface instances
`4` - Wind Injector demo

Only one Niagara showcase is active at a time.

## Wind Injector demo

The Wind Injector is placed in the map and injects velocity into the shared Wind Vector Field. When Demo `4` is selected, move the spectator pawn into the injector's radius to experience the sampled wind movement.

Useful properties on `BP_VecWindDemo_WindInjector`:

- **Velocity To Inject** — wind direction and strength.
- **Radius** — injected wind area, in Unreal world units.
- **Enable Injection** — enables or disables the injector.

## Notes

The demo map is intended as a quick functional and regression test. Performance depends on the active Niagara system, particle count, material, renderer settings, resolution, and hardware.