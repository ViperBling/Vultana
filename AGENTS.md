# Repository Guidelines

## Project Overview

**Vultana** is a real-time 3D rendering engine built on Vulkan with a custom RHI (Rendering Hardware Interface) abstraction layer. It features GPU-driven meshlet rendering, a DAG-based render graph, skeletal animation, and an ImGui-based editor.

- **Language**: C++20, HLSL Shader Model 6.6+
- **Platform**: Windows (Win32 API, MSVC toolchain)
- **Graphics**: Vulkan 1.3 (D3D12 backend stubbed)
- **RHI**: `VK_EXT_descriptor_buffer` (GPU-visible descriptor heap, no `vkDescriptorSet`)

## Architecture & Data Flow

```
wWinMain (Tests/WinMain.cpp)
  ├── Win32Window (Win32 + ImGui Win32 backend)
  └── VultanaEngine (singleton)
        ├── enkiTS TaskScheduler (CPU job system)
        ├── Renderer::RendererBase
        │     ├── RHI::RHIDeviceVK → VMA, vk::Device, descriptor/sampler allocators
        │     ├── ShaderCompiler (DXC runtime HLSL→SPIR-V)
        │     ├── ShaderCache / PipelineStateCache
        │     ├── GPUScene (GPU instance data, scene buffers)
        │     ├── RG::RenderGraph (DAG-based frame graph, resource aliasing)
        │     ├── ForwardBasePass / DeferredBasePass (GPU-driven meshlet pipeline)
        │     └── GPUDrivenDebugLine
        ├── Scene::World (scene graph, camera, static/skeletal meshes, lights)
        ├── Editor::VultanaEditor (ImGui dockspace, gizmo, outliner)
        └── AssetManager (ModelLoader via cgltf, TextureLoader, ResourceCache)
```

### Frame Loop
```
Tick() → Editor::NewFrame → Editor::Tick → World::Tick → RendererBase::RenderFrame
  RenderFrame: GPUScene::Update → BuildRenderGraph → BeginFrame (fence sync)
  → UploadResource (staging→GPU) → Render (compute flush, RG execute, backbuffer copy)
  → EndFrame (present, signal, submit)
```

### Triple Buffering
`RHI_MAX_INFLIGHT_FRAMES = 3` — ring-buffered command lists (graphics, async compute, upload queues), fences, and staging allocators.

## Key Directories

| Directory | Purpose |
|---|---|
| `Framework/Core/` | Engine singleton, lifecycle (`VultanaEngine`) |
| `Framework/RHI/` | Abstract RHI interfaces (pure virtual) |
| `Framework/RHI/RHIVulkan/` | Vulkan RHI implementation (`*VK` suffix) |
| `Framework/Renderer/` | Renderer, render graph, shader compilation, GPU scene |
| `Framework/Renderer/RenderGraph/` | DAG-based frame graph (DAG, RenderGraph, builder, resources) |
| `Framework/Renderer/ForwardPath/` | Forward+ rendering pass (GPU-driven meshlet) |
| `Framework/Renderer/RenderModules/` | HZB, GPU debug line rendering |
| `Framework/Renderer/RenderResources/` | Typed resource wrappers (Texture2D, buffers) |
| `Framework/Scene/` | World, camera, IVisibleObject, meshes, lights, animation |
| `Framework/Editor/` | ImGui editor (panels, gizmo, render graph viewer) |
| `Framework/AssetManager/` | GLTF loading (cgltf), texture loading, resource cache |
| `Framework/Window/` | Win32 and GLF window abstractions |
| `Framework/Utilities/` | Math (linalg+hlslpp), memory (rpmalloc), logging, hash, string |
| `Shaders/` | HLSL source files; `Shaders/Common/` for shared headers |
| `External/` | Vendored libraries (ImGui, tinyxml2, rpmalloc, OffsetAllocator, ImGuizmo, ImGuiNodeEditor, etc.) |
| `Assets/` | GLTF models, KTX textures, XML scene files, fonts |
| `Tests/` | Entry point (`WinMain.cpp`), test scaffold (GoogleTest) |
| `Config/` | Runtime INI config (`VultanaEngine.ini`, `ImGui.ini`) |
| `Tools/GraphViz/` | Render graph visualization export |

## Development Commands

### Build
```bash
# Configure with vcpkg toolchain
cmake -B CMakeBuild/windows-msvc-debug -S . -G Ninja \
  -DCMAKE_TOOLCHAIN_FILE=D:/Software/vcpkg/scripts/buildsystems/vcpkg.cmake \
  -DCMAKE_BUILD_TYPE=Debug

# Build
cmake --build CMakeBuild/windows-msvc-debug

# Output: Binary/VultanaEngine.exe
```

**Required toolchain**: MSVC (x64), Ninja generator, vcpkg classic mode (x64-windows triplet).

**Key defines** (set in `Framework/CMakeLists.txt`):
- `NOMINMAX`, `_CRT_SECURE_NO_WARNINGS`, `UNICODE`, `_UNICODE`
- `EASTL_EASTDC_VSNPRINTF=0`, `EASTL_USER_DEFINED_ALLOCATOR=1`

### Run
```bash
cd Binary/
./VultanaEngine.exe
```

Requires `dxcompiler.dll`, `vulkan-1.dll`, and runtime DLLs in `Binary/`. `Config/VultanaEngine.ini` must be present specifying `AssetsPath` and `ShaderPath`.

### Test
Tests use **GoogleTest** but are currently **commented out** in CMake. Re-enable by uncommenting `find_package(GTest)` and test targets in `Tests/CMakeLists.txt`.

## Code Conventions & Common Patterns

### Naming

| Category | Convention | Example |
|---|---|---|
| Namespaces | PascalCase | `Core`, `RHI`, `Renderer`, `Scene`, `RG` |
| Classes | PascalCase; RHI types prefixed `RHI` | `RHIDevice`, `RHIBufferVK`, `GPUScene` |
| Files | PascalCase matching class, `.hpp`/`.cpp` | `VultanaEngine.hpp`, `RHICommon.hpp` |
| Members | `m_` prefix; `m_p` for pointers, `m_b` for bools | `m_pDevice`, `m_FrameTime`, `m_bVSync` |
| Enums | `E` prefix, values share prefix | `ERHIFormat::RGBA8SRGB` |
| Constants | `UPPER_SNAKE_CASE` with namespace prefix | `RHI_MAX_INFLIGHT_FRAMES` |
| Macros | `VTNA_` prefix (VultaNA) | `VTNA_LOG_ERROR`, `VTNA_ALLOC` |

### Memory
- **No raw `new`/`delete`** — use `VTNA_ALLOC`/`VTNA_FREE` (rpmalloc) for dynamic allocation
- **Ownership**: `eastl::unique_ptr` for owning pointers; raw pointers for non-owning references
- **Frame scratch**: `LinearAllocator` (bump allocator) for per-frame temporary data
- **GPU sub-allocation**: `OffsetAllocator::Allocator` for packing into large GPU buffers

### Error Handling
- **Logging**: `VTNA_LOG_ERROR/WARN/INFO/DEBUG/TRACE` via spdlog (with `__FILE__`, `__LINE__`, `SPDLOG_FUNCTION`)
- **Assertions**: `assert()` for invariants and preconditions
- **Init returns `bool`**: `CreateDevice()`, `LoadScene()` return success/failure
- **No exceptions**: the codebase does not throw; fatal errors call `exit(0)` or log and return

### RAII Helpers
- `RHI::RenderEvent` / `GPU_EVENT_DEBUG(cmdList, name)` — scoped GPU profiling events
- `RG::RenderGraphEvent` / `RENDER_GRAPH_EVENT(graph, name)` — scoped render graph events
- `RHIDeletionQueueVK` — defers Vulkan object destruction by frame ID for GPU safety

### Singleton Pattern
`VultanaEngine::GetEngineInstance()` — Meyer's singleton. Also `ResourceCache::GetInstance()`.

### Dependency Injection
Constructor injection passes `RendererBase*`, `World*`, or `RHIDeviceVK*` to dependents. No formal DI container.

### Containers
Use **EASTL** everywhere: `eastl::string`, `eastl::vector`, `eastl::unique_ptr`, `eastl::hash_map`, `eastl::function`. No `std::` containers in engine code.

### Math Types
- `float2`, `float3`, `float4` from **linalg** (`linalg::aliases`)
- `float4x4` from linalg; `quaternion = float4`
- `hlslpp` functions for GPU-compatible operations (`Mul`, `Inverse`)
- **No** `glm` — it's commented out

### Shader Conventions
- **Language**: HLSL, compiled at runtime by DXC to SPIR-V (`-spirv -fspv-target-env=vulkan1.3`)
- **Binding model**: Vulkan bindless via SM 6.6 `ResourceDescriptorHeap`/`SamplerDescriptorHeap`
- **Register convention**: `b0` (root constants), `b1` (secondary, e.g. ImGui projection), `b2` (`SceneCB` global UBO)
- **Shared structs**: CPU/GPU shared via `#ifndef __cplusplus` guards in `.hlsli` headers
- **Shaders NOT compile-time**: `Shaders.cmake` marks files `HEADER_FILE_ONLY` (IDE grouping only)

### Render Graph Pattern
```cpp
auto& pass = graph.AddPass<FPassData>(name, RenderPassType::Graphics,
  [](FPassData& data, RGBuilder& builder) {
      // Declare reads/writes
  },
  [](const FPassData& data, RenderGraphPassExecuteContext& ctx) {
      // Execute: bind PSO, draw
  });
```

## Important Files

| File | Role |
|---|---|
| `Tests/WinMain.cpp` | Application entry point (`wWinMain`) |
| `CMakeLists.txt` | Root CMake (3.20+, C++20, Ninja) |
| `Framework/CMakeLists.txt` | FrameworkLib static library, all deps |
| `Framework/Core/VultanaEngine.hpp` | Engine singleton, owns all subsystems |
| `Framework/RHI/RHICommon.hpp` | RHI enums, structs, constants (~50 formats, access flags) |
| `Framework/RHI/RHI.hpp` | RHI umbrella header + `CreateRHIDevice` factory |
| `Framework/RHI/RHIVulkan/RHIDeviceVK.hpp` | Vulkan device (vk::Device, VMA, pipeline layout) |
| `Framework/Renderer/RendererBase.hpp` | Renderer: device, swapchain, PSO cache, shader compiler, render graph |
| `Framework/Renderer/GPUScene.hpp` | GPU scene data: instance buffer, static/animation buffer allocation |
| `Framework/Renderer/RenderGraph/RenderGraph.hpp` | DAG-based frame graph with resource aliasing |
| `Framework/Renderer/ForwardPath/ForwardBasePass.hpp` | GPU-driven forward rendering pass |
| `Framework/Scene/World.hpp` | Scene graph root (camera, objects, lights) |
| `Framework/Editor/VultanaEditor.hpp` | ImGui editor (dockspace, gizmo, panels) |
| `Framework/Utilities/Memory.hpp` | `VTNA_ALLOC`/`VTNA_FREE` macros (rpmalloc) |
| `Framework/Utilities/Log.hpp` | `VTNA_LOG_*` macros (spdlog) |
| `Framework/Utilities/Math.hpp` | `float3`/`float4x4` aliases, math helpers |
| `Framework/Utilities/ParallelFor.hpp` | `ParallelFor` via enkiTS |
| `Shaders/Common/GlobalConstants.hlsli` | `FSceneConstants` struct (CPU+GPU shared) |
| `Shaders/Common/GPUScene.hlsli` | `FInstanceData`, templated buffer access helpers |
| `Config/VultanaEngine.ini` | Runtime paths and default scene |

## Runtime/Tooling Preferences

- **Compiler**: MSVC (VS 2026), C++20, `/MP` (multi-process compilation)
- **Build generator**: Ninja
- **Package manager**: vcpkg (classic mode, `x64-windows` triplet, at `D:/Software/vcpkg`)
- **Shader compiler**: DirectXShaderCompiler (`dxcompiler.dll`) loaded dynamically at runtime
- **Memory allocator**: rpmalloc (replaces system allocator)
- **Vulkan loading**: Vulkan-Hpp (C++ bindings) + VMA
- **Non-MSVC**: `-fms-extensions` flag for Clang compatibility (not primary target)
- **Static CRT**: `/MTd` (Debug), `/MT` (Release)

## Testing & QA

- **Framework**: GoogleTest (`gtest/gtest.h`) with standard `testing::InitGoogleTest` + `RUN_ALL_TESTS()`
- **Current state**: Tests scaffolded but commented out; only `EngineTest.Init` (full lifecycle smoke test)
- **Test linking**: `GTest::gtest GTest::gtest_main GTest::gmock GTest::gmock_main` + `FrameworkLib`
- **No CI** configuration observed
- **No code coverage** tooling configured
