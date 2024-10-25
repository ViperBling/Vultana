set(AssetsPath ${PROJECT_SOURCE_DIR}/Assets)
set(BinaryPath ${PROJECT_SOURCE_DIR}/Binary)
set(SourcePath ${PROJECT_SOURCE_DIR}/Framework)
set(ExternalPath ${PROJECT_SOURCE_DIR}/External)

# find_package(assimp CONFIG REQUIRED)
# find_package(glfw3 CONFIG REQUIRED)
find_package(cityhash CONFIG REQUIRED)
find_package(EASTL CONFIG REQUIRED)
find_package(fmt CONFIG REQUIRED)
find_package(directx-dxc CONFIG REQUIRED)
# find_package(imguizmo CONFIG REQUIRED)
find_package(meshoptimizer CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_package(Vulkan REQUIRED)
find_package(VulkanHeaders CONFIG)

find_path(TCB_SPAN_INCLUDE_DIRS "tcb/span.hpp")
find_path(LINALG_INCLUDE_DIRS "linalg.h")
find_path(SIGSLOT_INCLUDE_DIRS "sigslot.h")
find_path(SIMPLEINI_INCLUDE_DIRS "ConvertUTF.c")

file(GLOB_RECURSE AssetManager_Head       ${SourcePath}/AssetManager/*.hpp)
file(GLOB_RECURSE AssetManager_Src        ${SourcePath}/AssetManager/*.cpp)
file(GLOB_RECURSE Common_Head             ${SourcePath}/Common/*.hpp)
file(GLOB_RECURSE Common_Src              ${SourcePath}/Common/*.cpp)
file(GLOB_RECURSE Core_Head               ${SourcePath}/Core/*.hpp)
file(GLOB_RECURSE Core_Src                ${SourcePath}/Core/*.cpp)
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
    ${AssetManager_Head} ${AssetManager_Src}
    ${Common_Head}       ${Common_Src}
    ${Core_Head}         ${Core_Src}
    ${Renderer_Head}     ${Renderer_Src}      ${Renderer_Inline}
    ${RHI_Head}          ${RHI_Src}
    ${Scene_Head}        ${Scene_Src}
    ${Window_Head}       ${Window_Src}
    ${Utilities_Head}    ${Utilities_Src}
)

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
)

target_link_libraries(FrameworkLib PRIVATE
    # assimp::assimp
    # glfw
    # glm::glm
    cityhash
    EASTL
    Microsoft::DirectXShaderCompiler
    fmt::fmt-header-only
    ImFileDialog
    ImGui
    ImGuizmo
    # imguizmo::imguizmo
    meshoptimizer::meshoptimizer
    OffsetAllocator
    RPMalloc
    spdlog::spdlog
    tinyxml2
    Vulkan::Vulkan
    Vulkan::Headers
)