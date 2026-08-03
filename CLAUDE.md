# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

> **Full agent guidelines live in `AGENTS.md`** — this file is the distilled operating guide and is kept accurate to the current code. If a detail is missing here, check `AGENTS.md` first. Note `AGENTS.md` can lag the code (e.g. it still says `ForwardPath/ForwardBasePass`; that pass was renamed to `DeferredPath/DeferredBasePass`).

## Project Overview

**Vultana** is a real-time 3D rendering engine built on Vulkan 1.3 with a custom RHI (Rendering Hardware Interface) abstraction layer. Language is C++20 (MSVC/VS 2026) with HLSL SM 6.6 shaders compiled **at runtime** by DXC (not at build time). Key features: GPU-driven meshlet rendering with two-phase occlusion culling, a DAG-based render graph with resource aliasing/barriers, a **GBuffer-producing GPU-driven base pass**, compute skinning, bindless via `VK_EXT_descriptor_buffer`, and an ImGui editor.

The D3D12 backend (`Framework/RHI/RHID3D/`) is a header-only stub — not functional. There is no multi-backend goal; Vulkan is the only backend.

## Build & Run

### Configure + Build (Windows, Ninja, vcpkg classic mode)

```powershell
# Configure (from project root)
cmake -G Ninja `
  -DCMAKE_TOOLCHAIN_FILE="D:/Softwares/vcpkg/scripts/buildsystems/vcpkg.cmake" `
  -DCMAKE_BUILD_TYPE=Debug `
  -DCMAKE_EXPORT_COMPILE_COMMANDS=TRUE `
  -B "CMakeBuild/Ninja/Visual Studio Community 2026 Release - amd64/Debug" `
  -S .

# Build
cmake --build "CMakeBuild/Ninja/Visual Studio Community 2026 Release - amd64/Debug"

# Output: Binary/VultanaEngine.exe
```

### Run

```powershell
cd Binary/
./VultanaEngine.exe
```

**Working directory MUST be `Binary/`.** The engine hardcodes `m_WorkingPath = "../"` and resolves `../Config/VultanaEngine.ini`, `../Assets/`, `../Shaders/` against it. Required runtime DLLs (`dxcompiler.dll`, `vulkan-1.dll`, `meshoptimizer.dll`, `spdlogd.dll`, `fmtd.dll`, `enkiTS.dll`) are copied into `Binary/` by vcpkg's `applocal.ps1` POST_BUILD step. If the ini fails to load, the engine logs an error and continues with empty asset/shader paths.

### Tests

Test targets are **commented out** in `Tests/CMakeLists.txt` (GoogleTest). There is no lint or CI. To re-enable: uncomment `find_package(GTest CONFIG REQUIRED)` and the `EngineTest` block (GTest must be installed via vcpkg), then run `EngineTest.exe` from `Binary/`. `Tests/EngineTest.cpp` is a full-lifecycle smoke test (GLFWindow + Init/Tick/Shutdown, pass = no crash).

## Architecture

### Engine Structure & Startup

```
wWinMain (Tests/WinMain.cpp)
  ├── rpmalloc_initialize()
  ├── Win32Window::Create()                 (HWND wrapper, NOT GLFW)
  └── VultanaEngine (Meyer's singleton)
        ├── enki::TaskScheduler             (CPU job system, rpmalloc-backed)
        ├── RendererBase                    (owns device, caches, render graph, GPUScene)
        │     ├── RHIDeviceVK               (vk::Device, VMA, 3 queues: graphics/compute/copy)
        │     ├── ShaderCompiler + ShaderCache + PipelineStateCache
        │     ├── GPUScene                  (GPU instance data, scene buffers)
        │     ├── RG::RenderGraph           (DAG frame graph, resource aliasing)
        │     ├── DeferredBasePass          (GPU-driven meshlet pass, writes GBuffer)
        │     ├── HiZBuffer                 (hierarchical Z for occlusion culling)
        │     └── GPUDrivenStats / GPUDrivenDebugLine
        ├── Scene::World                    (flat container: camera, objects, lights; XML scene)
        ├── VultanaEditor                   (ImGui dockspace, ImGuizmo, ImFileDialog)
        └── AssetManager                    (ModelLoader via cgltf, ResourceCache singleton)
```

### Frame Loop

`VultanaEngine::Tick()` → `Editor::NewFrame` → `Editor::Tick` → `World::Tick` (camera, frustum cull via `ParallelFor`) → `RendererBase::RenderFrame`:

`GPUScene::Update` → `BuildRenderGraph` → `BeginFrame` (fence wait, allocator reset) → `UploadResource` (staging→GPU copies on copy queue) → `Render` (debug line/stats clear → SetupGlobalConstants → FlushComputePass (skinning) → `RenderGraph::Execute` → stats readback → backbuffer copy + ImGui draw) → `EndFrame` (present, fence signal, reset staging/CB allocators).

### Render Graph Frame Composition (`RenderFrame.cpp::BuildRenderGraph`)

1. `ImportPrevFrameTextures` — import prev-frame depth/color (or clear on first frame)
2. `HiZBuffer::GenerateCullingHZB1stPhase` — HZB from previous depth
3. `DeferredBasePass::Render1stPhase` — instance culling vs prev-frame HZB, writes **GBuffer**; occluded meshlets queued
4. `HiZBuffer::GenerateCullingHZB2ndPhase` — HZB from current depth
5. `DeferredBasePass::Render2ndPhase` — render previously-occluded meshlets
6. `HiZBuffer::GenerateSceneHZB` — full-scene HZB
7. `OutlinePass` → 8. `ObjectIDPass` (R32UI, GPU→CPU readback for mouse picking) → 9. `CopyHistoryPass` → 10. `Present` → `Compile()`

The base pass (`DeferredPath/DeferredBasePass.hpp`) allocates a 4-target GBuffer — **DiffuseRT** (RGBA16F), **NormalRT** (RGBA8UNORM), **VelocityRT** (RG16F), **DepthRT** (D32F) — but only diffuse + depth are currently wired into the frame (`GetDiffuseRT()` is presented as scene color). Normal/velocity are written but unconsumed; there is **no lighting pass yet** (the roadmap's next step is clustered deferred lighting on this GBuffer).

### GPU-Driven Meshlet Pipeline

`InstanceCulling` (compute, 2-phase) → `BuildMeshletList` (compact culled meshlets) → `BuildIndirectCommand` (DispatchMesh args) → amplification shader (`MeshletCulling.hlsl::ASMain`: frustum + backface + HZB occlusion) → mesh shader (`ModelMeshlet.hlsl::MSMain`). Meshlets built at load time by `meshopt_buildMeshlets` (max 64 verts / 126 tris).

### Triple Buffering

`RHI_MAX_INFLIGHT_FRAMES = 3` (`Framework/RHI/RHICommon.hpp`). Per-frame ring buffers: graphics/compute/copy command lists, fences, `StagingBufferAllocator`s, GPUScene constant buffers, ImGui VB/IB, stats readback. Frame index = `frameID % 3`. `RHIDeletionQueueVK` defers Vulkan object destruction until the frame fence passes.

### RHI Abstraction

`Framework/RHI/RHI.hpp` — pure-virtual interfaces (`RHIDevice`, `RHIBuffer`, `RHICommandList`, …) with Vulkan impls (`RHIVulkan/*VK`) and a stub D3D12 backend. **Binding model**: `VK_EXT_descriptor_buffer` — GPU-visible descriptor heaps, all SRV/UAV via `ResourceDescriptorHeap[index]` / `SamplerDescriptorHeap[index]`, no `vkDescriptorSet` and no `register()` on individual resources.

## Code Conventions

- **Naming**: namespaces/classes PascalCase; RHI types prefixed `RHI`, Vulkan impls suffixed `VK`; members `m_` (`m_p` pointers, `m_b` bools); enums `E`-prefixed (flags use `Bit`); constants `UPPER_SNAKE_CASE`; macros `VTNA_`; shared CPU/GPU shader structs `F`-prefixed.
- **Memory**: no raw `new`/`delete` — use `VTNA_ALLOC`/`VTNA_FREE` (rpmalloc). Ownership via `eastl::unique_ptr`; raw pointers are non-owning. Frame scratch via `LinearAllocator` (bump, reset per frame); GPU sub-allocation via `OffsetAllocator`; staging via `StagingBufferAllocator`.
- **Error handling**: `VTNA_LOG_*` (spdlog), `assert()` for invariants, init functions return `bool`. **No exceptions anywhere** — no `try`/`catch`.
- **Containers & math**: EASTL exclusively (`eastl::vector/string/hash_map/function/atomic`, no `std::` containers in engine code). Math via **linalg** (`float2/3/4`, `float4x4`, quaternion = `float4`); hlslpp behind `ENABLE_HLSLPP` for `Mul`/`Inverse`. No glm.
- **Singletons**: Meyer's (`VultanaEngine::GetEngineInstance()`, `ResourceCache::GetInstance()`); dependent subsystems take constructor injection. No DI container.
- **RAII helpers**: `GPU_EVENT_DEBUG(cmdList, name)`, `RENDER_GRAPH_EVENT(graph, name)` (scoped profiling), `RHIDeletionQueueVK::Delete<T>(handle)` (frame-deferred destroy), `NOCOPY(ClassName)`.

### Render Graph Pattern

```cpp
auto& pass = graph.AddPass<FPassData>(name, RenderPassType::Graphics,
  [](FPassData& data, RGBuilder& builder) {
      // Declare reads/writes: builder.Read / Write / WriteColor / WriteDepth / ReadDepth
  },
  [](const FPassData& data, RenderGraphPassExecuteContext& ctx) {
      // Execute: bind PSO, draw
  });
```

Each `Write()` creates a new versioned resource node; `Compile()` culls unreferenced DAG nodes, allocates/aliases transient resources by lifetime overlap, and resolves barriers; `Execute()` traverses the DAG inserting barriers and cross-queue fences.

### AddGUICommand Pattern

```cpp
GUICommand("Window Name", [this]() { /* ImGui calls */ });
```

`Framework/Utilities/GUIUtil.hpp` routes the lambda to `VultanaEditor::AddGUICommand`, drawn per-window during editor `Tick`.

### Shader Conventions

- HLSL SM 6.6, compiled at runtime by DXC: `-spirv -fspv-target-env=vulkan1.3 -fvk-use-dx-layout -HV 2021 -enable-16bit-types`; bindless heaps `-fvk-bind-resource-heap 0 1`, `-fvk-bind-sampler-heap 0 2`; `-O0`/`-Zi` Debug, `-O3` Release; `RHI_BACKEND_VULKAN=1` always defined.
- **Register convention**: `b0` = root/per-pass constants, `b1` = secondary (ImGui projection, SPD constants), `b2` = `SceneCB` (global `FSceneConstants`).
- Shared CPU/GPU structs in `Shaders/Common/*.hlsli` behind `#ifndef __cplusplus` guards; CPU code includes the same headers.

## Key Files & Directories

| Path | Role |
|---|---|
| `Tests/WinMain.cpp` | Executable entry point (`wWinMain`, uses Win32Window) |
| `Framework/Core/VultanaEngine.hpp/.cpp` | Engine singleton; Init/Tick/Shutdown; owns all subsystems |
| `Framework/Renderer/RendererBase.hpp/.cpp` | Renderer: device, swapchain, caches, GPUScene, RenderGraph, triple-buffered cmd lists |
| `Framework/Renderer/RenderFrame.cpp` | `BuildRenderGraph()` — entire frame pass composition |
| `Framework/Renderer/DeferredPath/DeferredBasePass.hpp/.cpp` | Active GPU-driven 2-phase meshlet pass (writes GBuffer) |
| `Framework/Renderer/RenderGraph/RenderGraph.hpp` | Render graph: AddPass/Compile/Execute/Present |
| `Framework/Renderer/RenderGraph/DAG.hpp` | DAG with ref-count culling + GraphViz export |
| `Framework/Renderer/GPUScene.hpp` | GPU scene buffers (OffsetAllocator static buffer, animation buffer, per-frame CBs) |
| `Framework/Renderer/RenderModules/HiZBuffer.hpp` | HZB: 3 chains (1st/2nd-phase culling, scene), SPD-based |
| `Framework/Renderer/ShaderCompiler.hpp` | DXC (`IDxcCompiler3`) HLSL→SPIR-V wrapper |
| `Framework/Scene/World.hpp` | Flat scene container; XML scene loading; ParallelFor frustum culling |
| `Framework/Editor/VultanaEditor.hpp` | ImGui editor; `AddGUICommand` injection point; Tools → RenderGraph export |
| `Framework/AssetManager/ModelLoader.hpp` | cgltf GLTF loading + meshopt meshlet generation |
| `Framework/Utilities/Memory.hpp` / `Log.hpp` / `Math.hpp` | `VTNA_ALLOC` / `VTNA_LOG_*` / math aliases |
| `Shaders/Common/GlobalConstants.hlsli` | `FSceneConstants`, `SceneCB : register(b2)` |
| `Shaders/Common/GPUScene.hlsli` | `FInstanceData`, templated bindless buffer accessors |
| `Config/VultanaEngine.ini` | `[Vultana] AssetsPath/ShaderPath`, `[World] SceneFile` |

## Roadmap Context

`docs/superpowers/specs/2026-08-03-realengine-roadmap-design.md` documents the gap analysis vs the RealEngine reference (DX12 Ultimate) and the planned order: P0 (STBN blue noise + shader Print) → A1 (skybox/IBL) → **A2 GBuffer-ization of the meshlet pass (now done)** → A3 clustered deferred lighting → A4 post chain (tonemapper/AE/Bloom/TAA) → B (visibility buffer, meshlet LOD — Vultana's differentiation area) → C (GTAO, RT, ReSTIR GI). Recommended next step is A3 (clustered deferred lighting) on the existing GBuffer.

## Known Inconsistencies

- Namespace closing comments are stale in places (`Core/VultanaEngine.hpp` closes `} // namespace Vultana`, `RendererBase.cpp` closes `} // namespace Vultana::Renderer`) — residue of a namespace rename; actual namespaces are the short forms (`Core`, `Renderer`, `RG`, …).
- `Assets/Textures/EnvirnomentTex/` — directory name typo exists on disk; use as-is.
- `AGENTS.md` references `ForwardPath/ForwardBasePass`, which was renamed to `DeferredPath/DeferredBasePass` (see commit `cc969fa`); treat `AGENTS.md`'s description of the forward pass as describing the renamed deferred base pass.
