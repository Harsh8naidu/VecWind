# Physics-Driven Phoenix with Custom Wind Simulation in Unreal Engine 5

## Project Overview

This project was developed as part of my MSc dissertation in Computer Game Engineering at Newcastle University.
It implements a custom vector-grid wind simulation in Unreal Engine 5, integrated with both character physics and particle-based visual effects. The main focus is on real-time CPU–GPU data flow to synchronize gameplay systems with Niagara particle systems.

The project demonstrates how procedural wind can be used not only for environmental immersion but also as a physics-driven gameplay mechanic.

## Goals

- Implement a custom vector-field wind system.

- Drive phoenix character movement using wind-influenced physics.

- Build a custom Niagara Data Interface for CPU-to-GPU data transfer.

- Synchronize gameplay logic with Niagara GPU particle simulations.

- Integrate environmental effects such as flames, snow, and rain into the same wind system.

## Technical Implementation

- Engine: Unreal Engine 5

- Language: C++

- Custom Asset Type: UWindVectorField for storing and visualizing wind vectors.

- Niagara Integration:

  - Custom Data Interface for real-time CPU–GPU communication.

  - Wind vector sampling in Niagara for particle-based effects.

- Character Simulation:

  - Wind-driven physics applied to the phoenix skeletal mesh.

  - Movement and orientation influenced by procedural vector fields.

- Environmental Effects: Snow and rain particles synchronized with the wind system for consistency.

## Features

- Physics-driven phoenix character with wind-affected movement.

- Real-time vector-field wind simulation.

- Custom Niagara Data Interface for CPU–GPU synchronization.

- Unified system driving both character physics and particle-based visual effects.

## What I Learned

- Designing and registering custom Unreal Engine 5 asset types.

- Implementing Niagara Data Interfaces for efficient CPU–GPU data flow.

- Developing real-time vector-field simulations.

- Integrating physics-driven gameplay mechanics with particle effects.

- Managing performance trade-offs in real-time systems.

## Future Work

- Extend wind simulation to interact with environment geometry and terrain.

- Add gameplay mechanics where wind directly impacts player decisions.

- Expand phoenix behavior with AI responding to dynamic wind conditions.

## Acknowledgements

This project was completed as part of the MSc dissertation in Computer Game Engineering at Newcastle University (2025).
