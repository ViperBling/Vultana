find_package(Vulkan REQUIRED)

add_library(vkbootstrap STATIC)
target_sources(vkbootstrap PRIVATE 
    vkbootstrap/Vkbootstrap.h
    vkbootstrap/Vkbootstrap.cpp)
target_include_directories(vkbootstrap PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/vkbootstrap)
target_link_libraries(vkbootstrap PUBLIC Vulkan::Vulkan)

add_library(tinyxml2 STATIC)
target_sources(tinyxml2 PRIVATE 
    tinyxml2/tinyxml2.h
    tinyxml2/tinyxml2.cpp)
target_include_directories(tinyxml2 PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/tinyxml2)