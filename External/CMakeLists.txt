find_package(Vulkan REQUIRED)
find_package(Stb REQUIRED)

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

add_library(OffsetAllocator STATIC)
target_sources(OffsetAllocator PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/OffsetAllocator/OffsetAllocator.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/OffsetAllocator/OffsetAllocator.cpp)
target_include_directories(OffsetAllocator PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/OffsetAllocator)
set_target_properties(OffsetAllocator PROPERTIES FOLDER External CXX_STANDARD 20)

file (GLOB_RECURSE RPMalloc_Head ${CMAKE_CURRENT_SOURCE_DIR}/rpmalloc/*.h)
file (GLOB_RECURSE RPMalloc_Src ${CMAKE_CURRENT_SOURCE_DIR}/rpmalloc/*.c)

add_library(RPMalloc STATIC)
target_sources(RPMalloc PRIVATE ${RPMalloc_Head} ${RPMalloc_Src})
target_include_directories(RPMalloc PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/rpmalloc)

add_library(ImFileDialog STATIC ${CMAKE_CURRENT_SOURCE_DIR}/ImFileDialog/ImFileDialog.cpp)
target_include_directories(ImFileDialog PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/ImFileDialog 
    ${CMAKE_CURRENT_SOURCE_DIR}/ImGui
    ${Stb_INCLUDE_DIR}
)
target_link_libraries(ImFileDialog PUBLIC ImGui)

file (GLOB_RECURSE ImGUIZmo_Head ${CMAKE_CURRENT_SOURCE_DIR}/ImGuizmo/*.h)
file (GLOB_RECURSE ImGUIZmo_Src ${CMAKE_CURRENT_SOURCE_DIR}/ImGuizmo/*.cpp)
add_library(ImGuizmo STATIC)
target_sources(ImGuizmo PRIVATE ${ImGUIZmo_Head} ${ImGUIZmo_Src})
target_include_directories(ImGuizmo PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}/ImGuizmo 
    ${CMAKE_CURRENT_SOURCE_DIR}/ImGui)