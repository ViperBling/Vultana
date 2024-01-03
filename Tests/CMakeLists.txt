include_directories(${PROJECT_SOURCE_DIR}/Framework)

add_executable(EngineTest EngineTest.cpp)
target_link_libraries(EngineTest FrameworkLib)
target_include_directories(EngineTest PUBLIC ${PROJECT_SOURCE_DIR}/Framework)

add_executable(ShaderTest ShaderTest.cpp)
target_link_libraries(ShaderTest FrameworkLib)
target_include_directories(ShaderTest PUBLIC ${PROJECT_SOURCE_DIR}/Framework)

# add_executable(RenderGraphTest RenderGraphTest.cpp)
# target_link_libraries(RenderGraphTest FrameworkLib)
# target_include_directories(RenderGraphTest PUBLIC ${PROJECT_SOURCE_DIR}/Framework)