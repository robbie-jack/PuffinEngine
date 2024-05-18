# Puffin Engine

## What is Puffin Engine?
Puffin is a 3D Game Engine built in C++ which serves as a foundation for different features and techniques I want to experiment with, including ECS, Lighting, Ray Tracing, Procedural Generation, Physics, etc...

Assets & Shaders used while developing Puffin are located here: https://github.com/robbie-jack/PuffinProject

## Entity Component System & Scene Graph
Puffin is built around the use of Entity Component System (ECS) as a core design principle. This allows for writing performant, cache-optimzied code,
and makes it easier to share data between Puffin's various systems. Puffin also features a basic hierarchical scene graph built on top of the ecs,
structures around the concept of nodes. Nodes are comparable to Objects in commercial Game Engines such as Unity and Unreal, and are directly inspired by
Godot's Nodes. This allows for a more traditional Object Oriented Design (OOD) where appropriate. Neither Nodes or ECS are intended to be the primary way to use
Puffin, and both approaches have advantages and disadvantages.

## Rendering
Puffin's Renderer is built in Vulkan using a Bindless, GPU Driven Rendering approach. Puffin's renderer used uses modern vulkan features 
& extensions, including dynamic rendering, draw indirect, runtime descriptor arrays & more.

### What is GPU Driven Rendering
In a traditional rendering flow, the CPU issues draw calls for every mesh/material combination you want to draw in the scene. This results in
thousands of draw calls which makes the CPU the primary limiting factor in how fast you can render. In a GPU Driven approach, the amount of work
the CPU has to do is minimzed by batching together draw calls to, at most, one per material. This massively reduces the CPU burden, freeing up
performance to be used for other tasks, while allowing the GPU to run at full speed without having to wait for new commands from the CPU.

### What does 'Bindless' mean?
In a traditional renderer, the vertex & index buffers, materials, textures and other info required to render are bound for each draw call.
This is not a free process, and further burdens the CPU with issuing draw commands. With a bindless design, buffer & descriptor binding
is reduced to only a couple of calls a frame by merging data into large buffers and descriptor arrays, which can then by accessed in shaders by 
index. This couples well with a GPU Driven design, and allows you to build a fast renderer with minimal CPU performance requirements.

### To-Do

Non-Exhaustive list of various improvements & new features I wish to implement:

- Shadows
- Normal Mapping
- Ambient Occlusion
- Screen Space Reflections
- Temporal Anti-Aliasing
- Global Illumination
- HDR
- Clustered Light Culling
- Deferrred Shading/Rendering
- Physically Based Rendering
- Ray Tracing
- Double Precision Coordinates Support
- FidelityFX Super Resolution + Frame Generation
- Multithreading
- Compute-Based Frustrum/Occlusion Culling + other uses of Compute

## Physics
Puffin currently features basic implementations of third party physics engines, Box2D and JoltPhysics, as well as a custom physics solution called Onager.
JoltPhysics will likely be Puffin's default physics solution going forward, with Onager being developed mostly for learning purposes, though this may
change in the future.

Onager is currently 2D only and only supports basic shapes (Cirles and Boxes). Will eventually Implement a 3D option but this is a long term goal.

## In Progress

Working on Improving Puffin's Forward Renderer and adding new effects & features such as Shadows, Ambient Occlusion, etc... as well as general improvements
to the renderer's performance and usability.

### To-Do (Onager)

Non-Exhaustive list of improvements to Onager Physics sim:

- Continous Collision Detection
- Friction
- More Shapes
- Joints
- New Broadphases (BVH, CircleTree, QuadTree...)
- Double-Precision Coordinates Support
- 3D

## Third-Party Libraries

- Vulkan - Low Level Rendering API - https://www.vulkan.org/
- Vulkan-Hpp - C++ Vulkan Headers - https://github.com/KhronosGroup/Vulkan-Hpp
- VMA - Memory Allocation Library for Vulkan - https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator
- VMA-Hpp - C++ Bindings for VMA - https://github.com/YaaZ/VulkanMemoryAllocator-Hpp
- GLM (OpenGL Mathematics) - C++ Mathematics Library - https://github.com/icaven/glm
- GLFW - Window Creation/Management - https://www.glfw.org/
- ImGui - Editor UI - https://github.com/ocornut/imgui
- tinyobjloader - OBJ Loader - https://github.com/tinyobjloader/tinyobjloader
- tiny glTF - GLTF Loader - https://github.com/syoyo/tinygltf
- STB - Image Loader - https://github.com/nothings/stb
- AngelScript - Scripting - https://www.angelcode.com/angelscript/
- MiniAudio - Audio - https://miniaud.io/
- nlohmann/json - Json Parsing - https://github.com/nlohmann/json
- LZ4 - LZ4 Asset Compression - https://github.com/lz4/lz4
- Compressonator - Texture and 3D Model Compression - https://github.com/GPUOpen-Tools/compressonator
- Open Simplex Noise - C++ Noise Gen Library - https://github.com/deerel/OpenSimplexNoise
- Box2D - 2D Physics - https://box2d.org/
- Jolt - 3D Physics - https://github.com/jrouwe/JoltPhysics
- enkiTS - Multithreaded Task Scheduler - https://github.com/dougbinks/enkiTS
- EnTT - Entity Component System - https://github.com/skypjack/entt