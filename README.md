## Overview
Implementation of multiple rendering techniques in C++, using Vulkan API. All techniques have been implemented on a deferred pipeline.

## Ambient Occlusion
- Configurable Screen Space Ambient Occlusion (SSAO)
- Configurable Screen Space Directional Occlusion (SSDO)
<p align = "center">
  <img width="430" height="370" alt="SSAO" src="https://github.com/user-attachments/assets/1887f940-d8c4-41ef-9f21-962c23a69d02" />
  <img width="430" height="370" alt="SSDO" src="https://github.com/user-attachments/assets/cf711f8c-ca46-45c1-93f4-5a534b504442" />
</p>

## Anti-Aliasing
- Temporal Anti-Aliasing (TAA) with motion vectors and history buffer
- Multisample Anti-Aliasing (MSAA) 2x, 4x and 8x
- Fast Approximate Anti-Aliasing (FXAA)
<p align = "center">
  <img width="430" height="370" alt="No AA" src="https://github.com/user-attachments/assets/f9eafc6c-1ba5-4f3d-91a3-1ef1bda890c3" />
  <img width="430" height="370" alt="taa" src="https://github.com/user-attachments/assets/2cc443b9-5771-4f4f-a800-613f875544f5" />
</p>

## Shadow Mapping
- Slope-based bias
- Percentage-Closer Filtering (PCF)
- Cascaded Shadow Maps for directional lights
- Omnidirectional Shadow Maps for point lights
<p align = "center">
  <img width="430" height="471" alt="PCF" src="https://github.com/user-attachments/assets/8d3f74b0-e949-4795-b8d0-6de939579778" />
  <img width="430" height="471" alt="Omnidirectional shadows" src="https://github.com/user-attachments/assets/1d538119-e90c-402a-adaf-67313f4faa1b" />
</p>

## Real-time Ray Tracing
- Hard shadows
- Denoised soft shadows; realistic results with only four samples per pixel
- Self-occlusion corrections
- Reflections
- Dynamic acceleration structures (TLAS and BLAS)
<p align = "center">
  <img width="430" height="471" alt="Denoised soft shadows" src="https://github.com/user-attachments/assets/038f4769-f11b-4075-a2bc-7276889ebde8" />
  <img width="475" height="471" alt="Reflections" src="https://github.com/user-attachments/assets/57bdcc1e-fc77-4826-8a1d-2bb2b912a623" />
</p>

## Post-processing via ping-pong buffering
- Tone mapping + gamma correction (ACES approximation)
- Chromatic aberration
- Bloom
<p align = "center">
  <img width="430" height="370" alt="Chromatic aberration" src="https://github.com/user-attachments/assets/d5617b49-7380-4f2d-b123-b13a570e3d2c" />
  <img width="430" height="370" alt="Bloom)" src="https://github.com/user-attachments/assets/17d6dc25-db4a-4597-ac00-9b6d1db5b273" />
</p>

## Other features
- Physically Based Rendering (PBR) materials
- Depth prepass for early Z-test
<p align = "center">
  <img width="787" height="370" alt="PBR materials" src="https://github.com/user-attachments/assets/c4124b5c-d41a-4ea3-8f4a-1c5b1008ab9b" />
</p>

## Installation guide
- Make sure Vulkan SDK is installed and added to your system PATH
- Execute gen_prj.cmd
- Open RenderingTechniques.sln in Visual Studio
- The selected scene is configured as a command-line argument
