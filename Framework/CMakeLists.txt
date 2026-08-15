set(AssetsPath ${PROJECT_SOURCE_DIR}/Assets)
set(BinaryPath ${PROJECT_SOURCE_DIR}/Binary)
set(SourcePath ${PROJECT_SOURCE_DIR}/Framework)
set(ExternalPath ${PROJECT_SOURCE_DIR}/External)

find_package(assimp CONFIG REQUIRED)
find_package(glfw3 CONFIG REQUIRED)
find_package(cityhash CONFIG REQUIRED)
find_package(EASTL CONFIG REQUIRED)
find_package(enkiTS CONFIG REQUIRED)
find_package(EnTT CONFIG REQUIRED)
find_package(fmt CONFIG REQUIRED)
find_package(directx-dxc CONFIG REQUIRED)
find_package(meshoptimizer CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_package(Vulkan REQUIRED)
find_package(VulkanHeaders CONFIG)

find_path(TCB_SPAN_INCLUDE_DIRS "tcb/span.hpp")
find_path(LINALG_INCLUDE_DIRS "linalg.h")
find_path(SIGSLOT_INCLUDE_DIRS "sigslot.h")
find_path(SIMPLEINI_INCLUDE_DIRS "SimpleIni.h")

set(VULTANA_WITH_COOKER 1 CACHE BOOL "Enable in-engine cook-on-miss (links CookerLib; OFF = offline cook only)")

file(GLOB_RECURSE AssetCooker_Head          ${SourcePath}/AssetCooker/*.hpp)
file(GLOB_RECURSE AssetCooker_Src           ${SourcePath}/AssetCooker/*.cpp)

set(AssetCooker_RuntimeSrc
    ${SourcePath}/AssetCooker/AssetPathUtils.cpp
    ${SourcePath}/AssetCooker/AssetReader.cpp
    ${SourcePath}/AssetCooker/AssetValidate.cpp
    ${SourcePath}/AssetCooker/AssetWriter.cpp)
list(REMOVE_ITEM AssetCooker_Src ${AssetCooker_RuntimeSrc})

file(GLOB_RECURSE AssetManager_Head       ${SourcePath}/AssetManager/*.hpp)
file(GLOB_RECURSE AssetManager_Src        ${SourcePath}/AssetManager/*.cpp)
file(GLOB_RECURSE Common_Head             ${SourcePath}/Common/*.hpp)
file(GLOB_RECURSE Common_Src              ${SourcePath}/Common/*.cpp)
file(GLOB_RECURSE Core_Head               ${SourcePath}/Core/*.hpp)
file(GLOB_RECURSE Core_Src                ${SourcePath}/Core/*.cpp)
file(GLOB_RECURSE Editor_Head             ${SourcePath}/Editor/*.hpp)
file(GLOB_RECURSE Editor_Src              ${SourcePath}/Editor/*.cpp)
file(GLOB_RECURSE Renderer_Head           ${SourcePath}/Renderer/*.hpp)
file(GLOB_RECURSE Renderer_Src            ${SourcePath}/Renderer/*.cpp)
file(GLOB_RECURSE Renderer_Inline         ${SourcePath}/Renderer/*.inl)
file(GLOB_RECURSE RHI_Head                ${SourcePath}/RHI/*.hpp)
file(GLOB_RECURSE RHI_Src                 ${SourcePath}/RHI/*.cpp)
file(GLOB_RECURSE Scene_Head              ${SourcePath}/Scene/*.hpp)
file(GLOB_RECURSE Scene_Src               ${SourcePath}/Scene/*.cpp)
file(GLOB_RECURSE Window_Head             ${SourcePath}/Window/*.hpp)
file(GLOB_RECURSE Window_Src              ${SourcePath}/Window/*.cpp)
file(GLOB_RECURSE Utilities_Head          ${SourcePath}/Utilities/*.hpp)
file(GLOB_RECURSE Utilities_Src           ${SourcePath}/Utilities/*.cpp)

add_library(FrameworkLib STATIC
    ${AssetCooker_RuntimeSrc}
    ${AssetManager_Head}                  ${AssetManager_Src}
    ${Common_Head}                        ${Common_Src}
    ${Core_Head}                          ${Core_Src}
    ${Editor_Head}                        ${Editor_Src}
    ${Renderer_Head}                      ${Renderer_Src}      ${Renderer_Inline}
    ${RHI_Head}                           ${RHI_Src}
    ${Scene_Head}                         ${Scene_Src}
    ${Window_Head}                        ${Window_Src}
    ${Utilities_Head}                     ${Utilities_Src}
)

add_library(CookerLib STATIC ${AssetCooker_Head} ${AssetCooker_Src})

target_include_directories(CookerLib PUBLIC
    ${SourcePath} ${ExternalPath}
    ${TCB_SPAN_INCLUDE_DIRS}
    ${LINALG_INCLUDE_DIRS}
    ${SIGSLOT_INCLUDE_DIRS}
    ${SHADER_ROOT}
    ${SIMPLEINI_INCLUDE_DIRS})

target_compile_definitions(CookerLib PUBLIC
    EASTL_EASTDC_VSNPRINTF=0
    EASTL_USER_DEFINED_ALLOCATOR=1
    _CRT_SECURE_NO_WARNINGS
    NOMINMAX
)
if(TARGET ispc_texcomp)
    target_compile_definitions(CookerLib PUBLIC VULTANA_HAS_ISPC_TEXCOMP=1)
endif()
target_link_libraries(CookerLib PRIVATE
    assimp::assimp
    cityhash
    EASTL
    fmt::fmt-header-only
    meshoptimizer::meshoptimizer
    spdlog::spdlog
)
if(TARGET ispc_texcomp)
    target_link_libraries(CookerLib PRIVATE ispc_texcomp)
endif()

if(VULTANA_WITH_COOKER)
    target_link_libraries(FrameworkLib PUBLIC CookerLib)
endif()

target_include_directories(FrameworkLib PUBLIC
    ${SourcePath} ${ExternalPath} 
    ${TCB_SPAN_INCLUDE_DIRS} 
    ${LINALG_INCLUDE_DIRS} 
    ${SIGSLOT_INCLUDE_DIRS} 
    ${SHADER_ROOT}
    ${SIMPLEINI_INCLUDE_DIRS})

if(MSVC)
    add_definitions(/MP)
    add_definitions(-DUNICODE)
    add_definitions(-D_UNICODE)
    add_definitions(-D_SILENCE_STDEXT_ARR_ITERS_DEPRECATION_WARNING)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
else()
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fms-extensions -Wno-deprecated-builtins -Wno-nullability-completeness")
endif()

target_compile_definitions(FrameworkLib PUBLIC
    EASTL_EASTDC_VSNPRINTF=0
    EASTL_USER_DEFINED_ALLOCATOR=1
    _CRT_SECURE_NO_WARNINGS
    NOMINMAX
    VULTANA_WITH_COOKER=$<BOOL:${VULTANA_WITH_COOKER}>
)

target_link_libraries(FrameworkLib PRIVATE
    glfw
    # glm::glm
    cityhash
    EASTL
    enkiTS::enkiTS
    EnTT::EnTT
    Microsoft::DirectXShaderCompiler
    fmt::fmt-header-only
    ImFileDialog
    ImGui
    ImGuiNodeEditor
    ImGuizmo
    OffsetAllocator
    RPMalloc
    spdlog::spdlog
    tinyxml2
    Vulkan::Vulkan
    Vulkan::Headers
)

if(VULTANA_WITH_COOKER)
    target_link_libraries(FrameworkLib PRIVATE meshoptimizer::meshoptimizer)
endif()