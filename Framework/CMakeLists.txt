set(AssetsPath ${PROJECT_SOURCE_DIR}/Assets)
set(BinaryPath ${PROJECT_SOURCE_DIR}/Binary)
set(SourcePath ${PROJECT_SOURCE_DIR}/Framework)
set(ExternalPath ${PROJECT_SOURCE_DIR}/External)

# find_package(assimp CONFIG REQUIRED)
# find_package(glfw3 CONFIG REQUIRED)
find_package(Vulkan REQUIRED)
find_package(VulkanHeaders CONFIG)
find_package(directx-dxc CONFIG REQUIRED)
# find_package(glm CONFIG REQUIRED)
find_package(fmt CONFIG REQUIRED)
find_package(cityhash CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)
# find_package(imgui CONFIG REQUIRED)
find_package(imguizmo CONFIG REQUIRED)

find_path(TCB_SPAN_INCLUDE_DIRS "tcb/span.hpp")
find_path(LINALG_INCLUDE_DIRS "linalg.h")
find_path(SIGSLOT_INCLUDE_DIRS "sigslot.h")

file(GLOB_RECURSE AssetManager_Head       ${SourcePath}/AssetManager/*.hpp)
file(GLOB_RECURSE AssetManager_Src        ${SourcePath}/AssetManager/*.cpp)
file(GLOB_RECURSE Common_Head             ${SourcePath}/Common/*.hpp)
file(GLOB_RECURSE Common_Src              ${SourcePath}/Common/*.cpp)
file(GLOB_RECURSE Core_Head               ${SourcePath}/Core/*.hpp)
file(GLOB_RECURSE Core_Src                ${SourcePath}/Core/*.cpp)
file(GLOB_RECURSE Renderer_Head           ${SourcePath}/Renderer/*.hpp)
file(GLOB_RECURSE Renderer_Src            ${SourcePath}/Renderer/*.cpp)
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
    ${Renderer_Head}     ${Renderer_Src}
    ${RHI_Head}          ${RHI_Src}
    ${Scene_Head}        ${Scene_Src}
    ${Window_Head}       ${Window_Src}
    ${Utilities_Head}    ${Utilities_Src}
)

# file(GLOB_RECURSE ShaderSrc ${AssetsPath}/Shaders/GLSL/*.vert ${AssetsPath}/Shaders/GLSL/*.frag ${AssetsPath}/Shaders/GLSL/*.comp)

# add_custom_target(CompileShaders
#     COMMAND ${CMAKE_COMMAND} -E make_directory ${BinaryPath}/Assets/Shaders/Generated)

# foreach(Shader ${ShaderSrc})
#     get_filename_component(ShaderName ${Shader} NAME)
#     set(SPIRV_OUTPUT ${BinaryPath}/Assets/Shaders/Generated/${ShaderName}.spv)
#     add_custom_command(TARGET FrameworkLib PRE_BUILD
#         COMMAND glslangValidator -V ${Shader} -o ${SPIRV_OUTPUT} -t
#         # COMMENT "Compiling ${ShaderName}"
#         )
# endforeach()

# add_dependencies(FrameworkLib CompileShaders)

target_include_directories(FrameworkLib PUBLIC ${SourcePath} ${ExternalPath} ${TCB_SPAN_INCLUDE_DIRS} ${LINALG_INCLUDE_DIRS} ${SIGSLOT_INCLUDE_DIRS} ${SHADER_ROOT})
target_link_libraries(FrameworkLib PRIVATE
    # assimp::assimp
    # glfw
    # glm::glm
    Vulkan::Vulkan
    Vulkan::Headers
    fmt::fmt-header-only
    tinyxml2
    Microsoft::DirectXShaderCompiler
    cityhash
    spdlog::spdlog
    ImGui
    imguizmo::imguizmo
)