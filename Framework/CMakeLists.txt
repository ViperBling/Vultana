set(AssetsPath ${PROJECT_SOURCE_DIR}/Assets)
set(BinaryPath ${PROJECT_SOURCE_DIR}/Binary)
set(SourcePath ${PROJECT_SOURCE_DIR}/Framework)
set(ExternalPath ${PROJECT_SOURCE_DIR}/External)

find_package(assimp CONFIG REQUIRED)
find_package(glfw3 3.3 REQUIRED)
find_package(Vulkan REQUIRED)
find_package(VulkanMemoryAllocator CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)



file(GLOB_RECURSE Core_Src          ${SourcePath}/Core/*.cpp)
file(GLOB_RECURSE Renderer_Src      ${SourcePath}/Renderer/*.cpp)
file(GLOB_RECURSE RHI_Src           ${SourcePath}/RHI/*.cpp)
file(GLOB_RECURSE Scene_Src         ${SourcePath}/Scene/*.cpp)
file(GLOB_RECURSE Window_Src        ${SourcePath}/Windows/*.cpp)

add_library(FrameworkLib STATIC 
    ${Core_Src}
    ${Renderer_Src}
    ${RHI_Src}
    ${Scene_Src}
    ${Window_Src}
)

file(GLOB_RECURSE ShaderSrc ${AssetsPath}/Shaders/GLSL/*.vert ${AssetsPath}/Shaders/GLSL/*.frag ${AssetsPath}/Shaders/GLSL/*.comp)

add_custom_target(CompileShaders
    COMMAND ${CMAKE_COMMAND} -E make_directory ${BinaryPath}/Assets/Shaders/Generated)

foreach(Shader ${ShaderSrc})
    get_filename_component(ShaderName ${Shader} NAME)
    set(SPIRV_OUTPUT ${BinaryPath}/Assets/Shaders/Generated/${ShaderName}.spv)
    add_custom_command(TARGET FrameworkLib PRE_BUILD
        COMMAND glslangValidator -V ${Shader} -o ${SPIRV_OUTPUT} -t
        # COMMENT "Compiling ${ShaderName}"
        )
endforeach()

add_dependencies(FrameworkLib CompileShaders)

target_include_directories(FrameworkLib PUBLIC ${SourcePath} ${ExternalPath})
target_link_libraries(FrameworkLib PUBLIC
    # assimp::assimp
    glfw
    glm::glm
    Vulkan::Vulkan
    # GPUOpen::VulkanMemoryAllocator
    tinyxml2)