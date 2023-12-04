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
    
    ${ExternalPath}/tinyxml2/tinyxml2.cpp
)

target_include_directories(FrameworkLib PUBLIC ${SourcePath} ${ExternalPath})
target_link_libraries(FrameworkLib PUBLIC
    assimp::assimp
    glfw
    glm::glm
    Vulkan::Vulkan
    GPUOpen::VulkanMemoryAllocator)