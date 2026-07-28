# Repository Guidelines

## Project Overview

**Vultana** is a real-time 3D rendering engine built on Vulkan 1.3 with a custom RHI (Rendering Hardware Interface) abstraction layer. It features GPU-driven meshlet rendering with two-phase occlusion culling, a DAG-based render graph, skeletal animation, and an ImGui-based editor.

- **Language**: C++20 (engine), HLSL Shader Model 6.6 (shaders)
- **Platform**: Windows, MSVC toolchain (VS 2026), Ninja generator
- **Graphics**: Vulkan 1.3 via Vulkan-Hpp + VMA; D3D12 backend is a header-only stub (`Framework/RHI/RHID3D/`), not functional
- **RHI binding model**: `VK_EXT_descriptor_buffer` — GPU-visible descriptor heap, no `vkDescriptorSet`

## Architecture & Data Flow

```
wWinMain (Tests/WinMain.cpp)
  ├── rpmalloc_initialize()
  ├── Win32Window::Create()                     (HWND wrapper, NOT GLFW)
  └── VultanaEngine (Meyer's singleton)
        ├── enki::TaskScheduler                 (CPU job system, rpmalloc-backed)
        ├── Config/VultanaEngine.ini            (SimpleIni: AssetsPath, ShaderPath, SceneFile)
        ├── RendererBase
        │     ├── RHIDeviceVK                   (vk::Device, VMA, 3 queues: graphics/compute/copy)
        │     ├── ShaderCompiler (DXC runtime) + ShaderCache + PipelineStateCache
        │     ├── GPUScene                      (GPU instance data, scene buffers)
        │     ├── RG::RenderGraph               (DAG frame graph, resource aliasing)
        │     ├── ForwardBasePass               (GPU-driven meshlet pipeline, active)
        │     ├── HiZBuffer                     (hierarchical Z for occlusion culling)
        │     └── GPUDrivenStats / GPUDrivenDebugLine
        ├── Scene::World                        (flat container: camera, objects, lights; XML scene)
        ├── VultanaEditor                       (ImGui dockspace, ImGuizmo, ImFileDialog)
        └── AssetManager                        (ModelLoader via cgltf, ResourceCache singleton)
```

### Frame Loop

`VultanaEngine::Tick()` → `Editor::NewFrame` → `Editor::Tick` → `World::Tick` (camera, frustum cull via ParallelFor) → `RendererBase::RenderFrame`:

```
RenderFrame: GPUScene::Update → BuildRenderGraph → BeginFrame (fence wait, allocator reset)
  → UploadResource (staging→GPU copies on copy queue, texture barriers)
  → Render (debug line/stats clear → SetupGlobalConstants → FlushComputePass (skinning)
            → RenderGraph::Execute → stats readback → backbuffer copy + ImGui draw)
  → EndFrame (present, signal frame fence, reset staging/CB allocators)
```

### Render Graph Frame Composition (`RenderFrame.cpp::BuildRenderGraph`)

1. Import prev-frame depth/color (or clear on first frame)
2. `HiZBuffer::GenerateCullingHZB1stPhase` — HZB from previous depth
3. `ForwardBasePass::Render1stPhase` — instance culling vs prev-frame HZB, meshlet rendering
4. `HiZBuffer::GenerateCullingHZB2ndPhase` — HZB from current depth
5. `ForwardBasePass::Render2ndPhase` — render previously-occluded meshlets
6. `HiZBuffer::GenerateSceneHZB` — full scene HZB
7. OutlinePass → 8. ObjectIDPass (R32UI, GPU→CPU readback for mouse picking) → 9. CopyHistoryPass → 10. Present → `Compile()`

### GPU-Driven Meshlet Pipeline

`InstanceCulling` (compute, 2-phase) → `BuildMeshletList` (compact culled meshlets) → `BuildIndirectCommand` (DispatchMesh args) → amplification shader (`MeshletCulling.hlsl::ASMain`: frustum + backface + HZB occlusion) → mesh shader (`ModelMeshlet.hlsl::MSMain`). Meshlets are built at load time by `meshopt_buildMeshlets` (max 64 verts / 126 tris).

### Triple Buffering

`RHI_MAX_INFLIGHT_FRAMES = 3` (`Framework/RHI/RHICommon.hpp`). Ring-buffered per frame: graphics/compute/copy command lists, fences, `StagingBufferAllocator`s, `GPUScene` constant buffers, ImGui VB/IB, stats readback buffers, and `RHIDeviceVK` constant-buffer allocators. Frame index = `frameID % 3`. `RHIDeletionQueueVK` defers Vulkan object destruction until the frame fence passes.

## Key Directories

| Directory | Purpose |
|---|---|
| `Framework/Core/` | `VultanaEngine` singleton, lifecycle |
| `Framework/RHI/` | Abstract RHI interfaces (pure virtual); `RHIVulkan/` = `*VK` implementation; `RHID3D/` = stub |
| `Framework/Renderer/` | RendererBase, RenderFrame, GPUScene, shader compilation, PSO/shader caches, RenderBatch |
| `Framework/Renderer/RenderGraph/` | DAG frame graph (`RenderGraph`, `RGBuilder`, `DAG`, resource allocator) |
| `Framework/Renderer/ForwardPath/` | Active GPU-driven forward pass (2-phase) |
| `Framework/Renderer/DeferredPath/` | Deferred pass (exists, not wired into `RendererBase`) |
| `Framework/Renderer/RenderModules/` | HiZBuffer, GPUDrivenStats, debug line rendering |
| `Framework/Renderer/RenderResources/` | Typed resource wrappers (Texture2D, RawBuffer, StructuredBuffer, ConstantBuffer…) |
| `Framework/Scene/` | World container, camera, meshes, skeletal animation, lights |
| `Framework/Editor/` | ImGui editor (dockspace, gizmo, panels, `AddGUICommand`) |
| `Framework/AssetManager/` | GLTF loading (cgltf + meshoptimizer), texture loading, ResourceCache, materials |
| `Framework/Window/` | Win32Window (used by exe) and GLFWindow (GLFW, used by tests) |
| `Framework/Utilities/` | Memory (rpmalloc), Log (spdlog), Math (linalg+hlslpp), LinearAllocator, ParallelFor, Hash, KeyCodes |
| `Shaders/` | HLSL sources; `Shaders/Common/` = shared CPU/GPU `.hlsli` headers |
| `External/` | Vendored libraries (see Runtime/Tooling) |
| `Assets/` | glTF models, KTX textures, XML scene files, fonts, editor UI icons |
| `Tests/` | Entry point (`WinMain.cpp`) + commented-out GoogleTest scaffold |
| `Config/` | `VultanaEngine.ini` (runtime paths), `ImGui.ini` (docking layout) |
| `Tools/GraphViz/` | Viz.js (Graphviz WASM) for render graph DOT→SVG visualization |

## Development Commands

### Build

```powershell
# Configure (from project root; vcpkg classic mode, x64-windows triplet)
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

Shaders are **not** compiled at build time (`Shaders/Shaders.cmake` marks them `HEADER_FILE_ONLY`, IDE grouping only). DXC compiles them at runtime.

### Run

```powershell
cd Binary/
./VultanaEngine.exe
```

Working directory MUST be `Binary/`: the engine hardcodes `m_WorkingPath = "../"` and resolves `../Config/VultanaEngine.ini`, `../Assets/`, `../Shaders/` against it. Required DLLs (`dxcompiler.dll`, `vulkan-1.dll`, `meshoptimizer.dll`, `spdlogd.dll`, `fmtd.dll`, `enkiTS.dll`) are copied into `Binary/` by vcpkg's `applocal.ps1` POST_BUILD step. If the ini fails to load, an error is logged and execution continues with empty asset/shader paths.

### Test

Test targets are commented out — see Testing & QA below. There is no lint or CI configuration.

## Code Conventions & Common Patterns

### Naming

| Category | Convention | Example |
|---|---|---|
| Namespaces | PascalCase | `Core`, `RHI`, `Renderer`, `RG`, `Scene`, `Editor`, `Assets` |
| Classes | PascalCase; RHI types prefixed `RHI`, Vulkan impls suffixed `VK` | `RHIDevice`, `RHIBufferVK`, `GPUScene` |
| Files | PascalCase matching class, `.hpp`/`.cpp` | `VultanaEngine.hpp` |
| Members | `m_` prefix; `m_p` pointers, `m_b` bools | `m_pDevice`, `m_bVSync`, `m_FrameTime` |
| Enums | `E` prefix; flag enums use `Bit` suffix | `ERHIFormat`, `EBufferUsageBit` |
| Constants | `UPPER_SNAKE_CASE` | `RHI_MAX_INFLIGHT_FRAMES`, `VS_ENTRY_POINT` |
| Macros | `VTNA_` prefix | `VTNA_ALLOC`, `VTNA_LOG_ERROR` |
| Shader structs | `F` prefix (shared CPU/GPU) | `FSceneConstants`, `FInstanceData` |

### Memory

- **No raw `new`/`delete`** — `VTNA_ALLOC(size)` / `VTNA_ALLOC(size, align)` / `VTNA_REALLOC` / `VTNA_FREE` wrap rpmalloc (`Framework/Utilities/Memory.hpp`).
- **Ownership**: `eastl::unique_ptr` + `eastl::make_unique`; raw pointers are non-owning.
- **Frame scratch**: `LinearAllocator` (bump allocator, `Reset()` per frame) — used by RenderGraph, RenderBatch, CB allocators.
- **GPU sub-allocation**: `OffsetAllocator::Allocator` (GPUScene static buffer); **staging**: `StagingBufferAllocator` ring of CPU→GPU buffers, `Reset()` per frame.

### Error Handling

- `VTNA_LOG_TRACE/DEBUG/INFO/WARN/ERROR` via spdlog with source location (`Framework/Utilities/Log.hpp`).
- `assert()` for invariants; init functions (`CreateDevice`, `LoadScene`) return `bool`.
- **No exceptions** — no `try`/`catch` anywhere in the codebase.

### Containers & Math

- **EASTL exclusively**: `eastl::vector`, `eastl::string`, `eastl::hash_map`, `eastl::function`, `eastl::atomic`. No `std::` containers in engine code.
- Math: `float2/3/4`, `float4x4` from **linalg** (`using namespace linalg::aliases`); `quaternion = float4` typedef; hlslpp behind `ENABLE_HLSLPP` for `Mul`/`Inverse`. No glm.

### Singletons & Dependency Injection

- Meyer's singletons: `VultanaEngine::GetEngineInstance()`, `ResourceCache::GetInstance()`.
- Constructor injection for dependents: `VultanaEditor(RendererBase*)`, `GPUScene(RendererBase*)`, `RHIDeviceVK(RHIDeviceDesc)`. No DI container.

### RAII Helpers

- `GPU_EVENT_DEBUG(cmdList, name)` / `RHI::RenderEvent` — scoped GPU profiling events.
- `RENDER_GRAPH_EVENT(graph, name)` / `RG::RenderGraphEvent` — scoped render graph events.
- `RHIDeletionQueueVK::Delete<T>(handle)` — frame-deferred Vulkan object destruction.
- `NOCOPY(ClassName)` — deletes copy/move ctor + assignment.

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

`GUIUtil.hpp` routes the lambda to `VultanaEditor::AddGUICommand`, drawn per-window during editor `Tick`.

### Shader Conventions

- HLSL SM 6.6, compiled at runtime by DXC: `-spirv -fspv-target-env=vulkan1.3 -fvk-use-dx-layout -HV 2021 -enable-16bit-types`; bindless heaps `-fvk-bind-resource-heap 0 1`, `-fvk-bind-sampler-heap 0 2`; `-O0`/`-Zi` Debug, `-O3` Release; `RHI_BACKEND_VULKAN=1` always defined.
- **Binding model**: all SRV/UAV via `ResourceDescriptorHeap[index]`, samplers via `SamplerDescriptorHeap[index]` — no `register` on individual resources.
- **Register convention**: `b0` = root/per-pass constants, `b1` = secondary (ImGui projection, SPD constants), `b2` = `SceneCB` (global `FSceneConstants`).
- Shared structs live in `Shaders/Common/*.hlsli` behind `#ifndef __cplusplus` guards; CPU code includes the same headers.

### Known Inconsistencies

- Namespace closing comments are stale in places (`Core/VultanaEngine.hpp` closes `} // namespace Vultana`, `RendererBase.cpp` closes `} // namespace Vultana::Renderer`) — residue of a namespace rename; actual namespaces are the short forms.
- `Assets/Textures/EnvirnomentTex/` — directory name typo exists on disk; use as-is.

## Important Files

| File | Role |
|---|---|
| `Tests/WinMain.cpp` | Executable entry point (`wWinMain`, uses Win32Window) |
| `CMakeLists.txt` / `Framework/CMakeLists.txt` | Root CMake (3.20+, C++20) / `FrameworkLib` static lib with all deps + defines |
| `Shaders/Shaders.cmake` | Globs shaders as `HEADER_FILE_ONLY` for IDE grouping |
| `Framework/Core/VultanaEngine.hpp/.cpp` | Engine singleton; Init/Tick/Shutdown; owns all subsystems |
| `Framework/Renderer/RendererBase.hpp/.cpp` | Renderer: device, swapchain, caches, GPUScene, RenderGraph, triple-buffered cmd lists |
| `Framework/Renderer/RenderFrame.cpp` | `BuildRenderGraph()` — entire frame pass composition |
| `Framework/Renderer/RenderGraph/RenderGraph.hpp` | Render graph: AddPass/Compile/Execute/Present |
| `Framework/Renderer/RenderGraph/DAG.hpp` | DAG with ref-count culling + GraphViz export |
| `Framework/Renderer/GPUScene.hpp` | GPU scene buffers (OffsetAllocator static buffer, animation buffer, per-frame CBs) |
| `Framework/Renderer/ForwardPath/ForwardBasePass.hpp` | GPU-driven 2-phase meshlet forward pass |
| `Framework/Renderer/RenderModules/HiZBuffer.hpp` | HZB: 3 chains (1st/2nd-phase culling, scene), SPD-based |
| `Framework/Renderer/ShaderCompiler.hpp` | DXC (`IDxcCompiler3`) HLSL→SPIR-V wrapper |
| `Framework/Renderer/StagingBufferAllocator.hpp` | Per-frame CPU→GPU staging ring |
| `Framework/RHI/RHICommon.hpp` | All RHI enums/structs; `RHI_MAX_INFLIGHT_FRAMES = 3` |
| `Framework/RHI/RHI.hpp` / `RHI.cpp` | RHI umbrella header + `CreateRHIDevice` factory |
| `Framework/RHI/RHIVulkan/RHIDeviceVK.hpp` | Vulkan device: VMA, descriptor-buffer heaps, 3 queues, deletion queue |
| `Framework/Scene/World.hpp` | Flat scene container; XML scene loading; ParallelFor frustum culling |
| `Framework/Editor/VultanaEditor.hpp` | ImGui editor; `AddGUICommand` injection point; Tools → RenderGraph export |
| `Framework/AssetManager/ModelLoader.hpp` | cgltf GLTF loading + meshopt meshlet generation |
| `Framework/Utilities/Memory.hpp` / `Log.hpp` / `Math.hpp` | `VTNA_ALLOC` / `VTNA_LOG_*` / math aliases |
| `Shaders/Common/GlobalConstants.hlsli` | `FSceneConstants`, `SceneCB : register(b2)` |
| `Shaders/Common/GPUScene.hlsli` | `FInstanceData`, templated bindless buffer accessors |
| `Config/VultanaEngine.ini` | `[Vultana] AssetsPath/ShaderPath`, `[World] SceneFile` |

## Runtime/Tooling Preferences

- **Compiler**: MSVC (VS 2026), C++20, `/MP`; non-MSVC gets `-fms-extensions` (not primary).
- **Generator**: Ninja. **Package manager**: vcpkg **classic mode** (no manifest), `x64-windows` triplet, rooted at `D:/Softwares/vcpkg`.
- **CRT**: effectively dynamic (`/MDd` Debug) — `Framework/CMakeLists.txt` sets `CMAKE_MSVC_RUNTIME_LIBRARY` to static *after* `project()`, which does not take effect. DLLs are required at runtime.
- **Key defines** (`Framework/CMakeLists.txt`): `NOMINMAX`, `_CRT_SECURE_NO_WARNINGS`, `UNICODE`, `_UNICODE`, `_SILENCE_STDEXT_ARR_ITERS_DEPRECATION_WARNING`, `EASTL_EASTDC_VSNPRINTF=0`, `EASTL_USER_DEFINED_ALLOCATOR=1`.
- **Allocators**: rpmalloc replaces system malloc; spdlog logging; enkiTS task scheduler; DXC loaded via `LoadLibrary` at runtime.

### vcpkg Packages (linked)

| Package | CMake Target | Notes |
|---|---|---|
| cityhash | `cityhash` | static |
| EASTL | `EASTL` | static |
| enkiTS | `enkiTS::enkiTS` | DLL |
| fmt | `fmt::fmt-header-only` | `fmtd.dll` pulled by spdlog |
| directx-dxc | `Microsoft::DirectXShaderCompiler` | `dxcompiler.dll` at runtime |
| meshoptimizer | `meshoptimizer::meshoptimizer` | DLL |
| spdlog | `spdlog::spdlog` | DLL |
| Vulkan / VulkanHeaders | `Vulkan::Vulkan` / `Vulkan::Headers` | |
| Stb, simpleini, sigslot, linalg, tcb-span | `find_package`/`find_path` | header-only |

### Vendored External Libraries (`External/`)

- **STATIC libs** (in `External/CMakeLists.txt`): tinyxml2, ImGui (docking), OffsetAllocator, RPMalloc, ImFileDialog, ImGuizmo, ImGuiNodeEditor.
- **Header-only**: sokol (sokol_time), hlslpp, ddspp, cgltf, sigslot (duplicate of vcpkg), Im3D (sources present, not in CMake).

## Testing & QA

- **Framework**: GoogleTest, but **all test targets are commented out** in `Tests/CMakeLists.txt`. Only the production `VultanaEngine` executable builds.
- **Existing test**: `Tests/EngineTest.cpp` — single `TEST(EngineTest, Init)` full-lifecycle smoke test (GLFWindow + Init/Tick/Shutdown loop; pass = no crash). `Tests/MainTest.cpp` is the gtest main.
- **Placeholders only**: `ShaderTest` and `RenderGraphTest` appear in commented CMake but have **no source files**.
- **To re-enable**: in `Tests/CMakeLists.txt` uncomment `find_package(GTest CONFIG REQUIRED)` (line 13) and the `EngineTest` block (lines 15-18); GTest must be installed via vcpkg. Run `EngineTest.exe` from `Binary/`.
- **No ctest** (`enable_testing()`/`add_test()` never called), **no CI**, **no coverage tooling**.
