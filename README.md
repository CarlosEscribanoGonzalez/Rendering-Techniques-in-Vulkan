## Project Information
* **Creation date**: May 20, 2026
* **Context:** Master's Degree Project

## Overview
Implementation of multiple rendering techniques in C++, using Vulkan API. All techniques have been implemented on a deferred pipeline.

## Features
### Ambient Occlusion
- Configurable Screen Space Ambient Occlusion (SSAO)
- Configurable Screen Space Directional Occlusion (SSDO)
<p align = "center">
  <img width="800" height="450" alt="AO" src="https://github.com/user-attachments/assets/956404cf-59c2-4d9c-861d-46cbe96d68f9" />
</p>

### Anti-Aliasing
- Temporal Anti-Aliasing (TAA) with motion vectors and history buffer
- Multisample Anti-Aliasing (MSAA) 2x, 4x and 8x
- Fast Approximate Anti-Aliasing (FXAA)
<p align = "center">
  <img width="800" height="450" alt="AA" src="https://github.com/user-attachments/assets/f16d1eef-6ab4-4f95-805f-63a8d3661f91" />
</p>

### Shadow Mapping
- Slope-based bias
- Percentage-Closer Filtering (PCF)
- Cascaded Shadow Maps for directional lights
- Omnidirectional Shadow Maps for point lights
<p align = "center">
  <img width="800" height="450" alt="PCF" src="https://github.com/user-attachments/assets/6270fb63-7e20-4e14-85b5-958ae83ab937" />
</p>

### Real-time Ray Tracing
- Hard shadows
- Denoised soft shadows; realistic results with only four samples per pixel
- Self-occlusion corrections
- Reflections
- Dynamic acceleration structures (TLAS and BLAS)
<p align = "center">
  <img width="800" height="450" alt="RT Shadows" src="https://github.com/user-attachments/assets/251a111f-7232-4121-bea6-f8ebfe42b174" />
</p>

### Post-processing via ping-pong buffering
- Tone mapping + gamma correction (ACES approximation)
- Chromatic aberration
- Bloom
<p align = "center">
  <img width="400" height="370" alt="Chromatic aberration" src="https://github.com/user-attachments/assets/39fa98ca-d73a-4676-9068-88f50ca9f055" />
  <img width="400" height="370" alt="Bloom" src="https://github.com/user-attachments/assets/950e20e9-2bb0-4fc2-afee-0d85ce183a46" />
</p>

### Other features
- Physically Based Rendering (PBR) materials
- Depth prepass for early Z-test
<p align = "center">
  <img width="800" height="385" alt="PBR Materials" src="https://github.com/user-attachments/assets/9dd87260-7d84-4301-ac2f-498884e4d629" />
</p>

## Technologies
* C++
* Vulkan
* GLFW
* GLM
* ImGUI

## Requirements
* Vulkan SDK
* CMake

## Usage
- Execute `gen_prj.cmd`
- Open `RenderingTechniques.sln` in Visual Studio 2022 or later (found in the `build` folder)
- The selected scene is configured as a command-line argument
