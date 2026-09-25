## Overview
Implementation of multiple rendering techniques in C++, using Vulkan API. All techniques have been implemented on a deferred pipeline.

## Ambient Occlusion
- Configurable Screen Space Ambient Occlusion (SSAO)
- Configurable Screen Space Directional Occlusion (SSDO)

## Anti-Aliasing
- Temporal Anti-Aliasing (TAA) with motion vectors and history buffer
- Multisample Anti-Aliasing (MSAA) 2x, 4x and 8x
- Fast Approximate Anti-Aliasing (FXAA)

## Shadow Mapping
- Slope-based bias
- Percentage-Closer Filtering (PCF)
- Cascaded Shadow Maps for directional lights
- Omnidirectional Shadow Maps for point lights

## Real-time Ray Tracing
- Hard shadows
- Denoised soft shadows; realistic results with only four samples per pixel
- Self-occlusions corrections
- Reflections
- Dynamic acceleration structures (TLAS and BLAS)

## Post-processing via ping-pong buffering
- Tone mapping + gamma correction (ACES approximation)
- Chromatic aberration
- Bloom

## Other features
- Physically Based Rendering (PBR) materials
- Depth prepass for early Z-test

## Installation guide
- Make sure Vulkan SDK is installed and added to your system PATH
- Execute gen_prj.cmd
- Open RenderingTechniques.sln in Visual Studio
- The selected scene is configured as a command-line argument
