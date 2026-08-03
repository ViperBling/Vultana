# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

> **Full agent guidelines live in `AGENTS.md`** — this file is the distilled operating guide and is kept accurate to the current code. If a detail is missing here, check `AGENTS.md` first. Two places where `AGENTS.md` currently lags the code: it says `ForwardPath/ForwardBasePass` (renamed to `DeferredPath/DeferredBasePass` in `cc969fa`) and it says vcpkg **classic** mode (the project moved to **manifest** mode in `4b7199f`). Trust this file over `AGENTS.md` on both.

## Project Overview

**Vultana** is a real-time 3D rendering engine built on Vulkan 1.3 with a custom RHI (Rendering Hardware Interface) abstraction layer. Language is C++20 (MSVC/VS 2026) with HLSL SM 6.6 shaders compiled **at runtime** by DXC (not at build time). Key features: GPU-driven meshlet rendering with two-phase occlusion culling, a DAG-based render graph with resource aliasing/barriers, a **GBuffer-producing GPU-driven base pass**, compute skinning, bindless via `VK_EXT_descriptor_buffer`, and an ImGui editor.

The D3D12 backend (`Framework/RHI/RHID3D/`) is a header-only stub — not functional. There is no multi-backend goal; Vulkan is the only backend.

## Build & Run

### Configure + Build (Windows, Ninja, vcpkg manifest mode)

Use the presets in `CMakePresets.json`. `CMake/VultanaVcpkg.cmake` runs *before* `project()` and locates vcpkg itself (via `CMAKE_TOOLCHAIN_FILE`, else `VCPKG_ROOT`, else `vcpkg` on `PATH`, else the `VULTANA_VCPKG_ROOT` cache var). It hard-forces `VCPKG_INSTALLED_DIR` to `<repo>/.vcpkg_installed` — dependencies from `vcpkg.json` install there, never into vcpkg's global `installed/`.

**Configure requires a shell where `cl.exe` is on `PATH`** — the presets pin `CMAKE_C/CXX_COMPILER` to `cl.exe`, so a plain shell fails with "compiler not found". Ninja does *not* need to be on `PATH`: `VultanaVcpkg.cmake` falls back to `vswhere` → the VS-bundled `ninja.exe`.

```bash
# Configure (from project root, in an environment that has cl.exe + ninja on PATH)
cmake --preset x64-windows-debug        # → CMakeBuild/Ninja/Debug
cmake --preset x64-windows-release      # → CMakeBuild/Ninja/Release

# Build — works from any shell once the tree is configured (verified)
cmake --build --preset x64-windows-debug

# Output: Binary/VultanaEngine.exe  (single target: VultanaEngine → FrameworkLib)
```

`CMakeBuild/Ninja/Visual Studio Community 2026 Release - amd64/` is a stale pre-preset build tree that still exists on disk; prefer the preset paths above. `compile_commands.json` lands in the preset binary dir.

Shaders are **not** compiled at build time — `Shaders/Shaders.cmake` globs them as `HEADER_FILE_ONLY` purely for IDE grouping. Editing a `.hlsl`/`.hlsli` needs no rebuild; just restart the engine (DXC compiles on load). Adding/removing a `.cpp` in `Framework/` **does** need a reconfigure — `Framework/CMakeLists.txt` uses `file(GLOB_RECURSE)`.

### Run

```bash
cd Binary/
./VultanaEngine.exe
```

**Working directory MUST be `Binary/`.** The engine hardcodes `m_WorkingPath = "../"` and resolves `../Config/VultanaEngine.ini`, `../Assets/`, `../Shaders/` against it. Required runtime DLLs (`dxcompiler.dll`, `vulkan-1.dll`, `meshoptimizer.dll`, `spdlogd.dll`, `fmtd.dll`, `enkiTS.dll`) are copied into `Binary/` by vcpkg's `applocal.ps1` POST_BUILD step. If the ini fails to load, the engine logs an error and continues with empty asset/shader paths.

Startup scene comes from `[World] SceneFile` in `Config/VultanaEngine.ini`. Two exist: `Scene_Sponza.xml` (default) and `Scene_SingleModelTest.xml` — switch to the latter for a much faster load when iterating on rendering code.

### Tests

There is **no working test target, no lint, and no CI.** Everything in `Tests/CMakeLists.txt` except the `VultanaEngine` executable is commented out (GoogleTest), and `enable_testing()`/`add_test()` are never called, so there is no ctest.

- `Tests/EngineTest.cpp` — the one real test: a single `TEST(EngineTest, Init)` full-lifecycle smoke test (GLFWindow + Init/Tick/Shutdown, pass = no crash). `Tests/MainTest.cpp` is the gtest main.
- `ShaderTest` / `RenderGraphTest` appear in the commented CMake but **have no source files** — those blocks cannot be uncommented as-is.
- To re-enable the real one: uncomment `find_package(GTest CONFIG REQUIRED)`, the `set(GTestLib ...)`/`set(MainFile ...)` lines, and the `EngineTest` block (add `gtest` to `vcpkg.json` first), then run `EngineTest.exe` from `Binary/`. Commented `add_custom_target(RunEngineTest COMMAND EngineTest)` provides a build-system entry point.


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

- HLSL SM 6.6, compiled at runtime by DXC (`ShaderCompiler.cpp`): `-spirv -fspv-target-env=vulkan1.3 -fvk-use-dx-layout -HV 2021 -enable-16bit-types`; bindless heaps `-fvk-bind-counter-heap 0 0`, `-fvk-bind-resource-heap 0 1`, `-fvk-bind-sampler-heap 0 2`; `-O0`/`-Zi -Qembed_debug` Debug, `-O3` Release; `RHI_BACKEND_VULKAN=1` always defined.
- **Hot reload**: editor menu **Debug → Reload Shaders** (`RendererBase::ReloadShaders` → `ShaderCache::ReloadShaders` → `RecompileShader` → `PipelineStateCache::RecreatePSO`) recompiles changed `.hlsl` files *and* every shader that `#include`s a changed `.hlsli` (`ShaderCache::IsFileIncluded` walks the include tree). No engine restart needed for shader edits.
- On a compile error, `ShaderCompiler::Compile` logs DXC's diagnostics via `VTNA_LOG_ERROR` and returns false; `ShaderCache::GetShader` then returns `nullptr` and `RecompileShader` keeps the old blob. Check the log first when a draw disappears.
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
| `CMakePresets.json` / `CMake/VultanaVcpkg.cmake` | Ninja+manifest presets / pre-`project()` vcpkg + Ninja auto-discovery |
| `Tools/GraphViz/RenderGraph.html` | Viz.js viewer for the DOT dumped by editor **Tools → RenderGraph** |
| `docs/superpowers/specs/`, `AIOut/` | Design specs (roadmap) and scratch AI-generated notes (`ECSDesign.md`, `OutDiff.md`) — not authoritative code docs |

## Roadmap Context

`docs/superpowers/specs/2026-08-03-realengine-roadmap-design.md` documents the gap analysis vs the RealEngine reference (DX12 Ultimate) and the planned order: P0 (STBN blue noise + shader Print) → A1 (skybox/IBL) → **A2 GBuffer-ization of the meshlet pass (now done)** → A3 clustered deferred lighting → A4 post chain (tonemapper/AE/Bloom/TAA) → B (visibility buffer, meshlet LOD — Vultana's differentiation area) → C (GTAO, RT, ReSTIR GI). Recommended next step is A3 (clustered deferred lighting) on the existing GBuffer.

## Known Inconsistencies

Do not "fix" these incidentally — they are load-bearing or harmless, and several are traps that look like bugs:

- Namespace closing comments are stale in places (`Core/VultanaEngine.hpp` closes `} // namespace Vultana`, `RendererBase.cpp` closes `} // namespace Vultana::Renderer`) — residue of a namespace rename; actual namespaces are the short forms (`Core`, `Renderer`, `RG`, …).
- `Assets/Textures/EnvirnomentTex/` — directory name typo exists on disk; use as-is.
- `AGENTS.md` references `ForwardPath/ForwardBasePass` (renamed to `DeferredPath/DeferredBasePass`, commit `cc969fa`) and vcpkg classic mode (now manifest, commit `4b7199f`).
- `Framework/Renderer/ForwardPath/` is an **empty directory** left by that rename. `Framework/CMakeLists.txt` also globs a `Framework/Common/` that does not exist (empty glob, harmless).
- `ShaderCompiler.cpp:185` — the `-O2` branch is `else if (flags & RHIShaderCompileFlagO3)`, a duplicate of the `-O3` condition above it, so `-O2` is unreachable dead code.
- **Never write `$ENV{ProgramFiles(x86)}` in CMake** — policy `CMP0053` (NEW for any project declaring `cmake_minimum_required(VERSION 3.1+)`, so always active here) rejects `(` in a variable name and turns the whole file into a syntax error. `CMake/VultanaVcpkg.cmake` looks the name up indirectly via `VULTANA_PROGRAM_FILES_X86_VAR`; keep it that way.
- **Configure trap** — a configure that aborts partway **deletes `<build-dir>/CMakeFiles/rules.ninja`** before failing, which leaves a previously-working tree broken (`cmake --build` then fails with `loading 'CMakeFiles\rules.ninja'`). Recovery is one successful reconfigure. Don't run speculative `cmake --preset` against a build tree you care about.
- `Framework/CMakeLists.txt:68` sets `CMAKE_MSVC_RUNTIME_LIBRARY` *after* `add_library(FrameworkLib)`, so it never applies — the build is effectively `/MDd`, which is why the runtime DLLs in `Binary/` are required.
- `vcpkg.json` lists `entt` and `glfw3`, but there is **no ECS in the engine** (no `entt` include anywhere; `AIOut/ECSDesign.md` is an unimplemented design sketch) and GLFW is only used by the commented-out test window.
- `Framework/RHI/RHID3D/` is 13 headers with no `.cpp` — a stub. Don't extend it; Vulkan is the only backend.

