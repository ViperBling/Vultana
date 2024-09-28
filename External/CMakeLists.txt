find_package(Vulkan REQUIRED)

add_library(tinyxml2 STATIC)
target_sources(tinyxml2 PRIVATE 
    tinyxml2/tinyxml2.h
    tinyxml2/tinyxml2.cpp)
target_include_directories(tinyxml2 PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/tinyxml2)

file (GLOB_RECURSE ImGUI_Head ${CMAKE_CURRENT_SOURCE_DIR}/ImGui/*.h)
file (GLOB_RECURSE ImGUI_Src ${CMAKE_CURRENT_SOURCE_DIR}/ImGui/*.cpp)

add_library(ImGui STATIC)
target_sources(ImGui PRIVATE ${ImGUI_Head} ${ImGUI_Src})
target_include_directories(ImGui PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/ImGui)
