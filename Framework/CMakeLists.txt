set(SourcePath ${PROJECT_SOURCE_DIR}/Framework)

find_package(assimp CONFIG REQUIRED)
find_package(glfw3 3.3 REQUIRED)
find_package(Vulkan REQUIRED FATAL_ERROR)
find_package(glm CONFIG REQUIRED)

message(STATUS "Found Vulkan: ${Vulkan_INCLUDE_DIRS}")

set(UTILITIES_SOURCE
    ${SourcePath}/Utilities/AssetsManager.cpp
    ${SourcePath}/Utilities/GLFWindow.cpp)

add_library(Utilities STATIC ${UTILITIES_SOURCE})
target_link_libraries(Utilities PUBLIC
    assimp::assimp
    glfw
    glm::glm)