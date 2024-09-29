include_directories(${PROJECT_SOURCE_DIR}/Framework)

find_package(GTest CONFIG REQUIRED)

set(GTestLib GTest::gtest GTest::gtest_main GTest::gmock GTest::gmock_main)
set(MainFile MainTest.cpp)

add_executable(EngineTest ${MainFile} EngineTest.cpp ${SHADER_FILES})
target_link_libraries(EngineTest FrameworkLib ${GTestLib})
target_include_directories(EngineTest PUBLIC ${PROJECT_SOURCE_DIR}/Framework)

# add_executable(ShaderTest ${MainFile} ShaderTest.cpp)
# target_link_libraries(ShaderTest FrameworkLib ${GTestLib})
# target_include_directories(ShaderTest PUBLIC ${PROJECT_SOURCE_DIR}/Framework)

# add_executable(RenderGraphTest ${MainFile} RenderGraphTest.cpp)
# target_link_libraries(RenderGraphTest FrameworkLib ${GTestLib})
# target_include_directories(RenderGraphTest PUBLIC ${PROJECT_SOURCE_DIR}/Framework)

add_custom_target(RunEngineTest COMMAND EngineTest)
# add_custom_target(RunShaderTest COMMAND ShaderTest)
# add_custom_target(RunRenderGraphTest COMMAND RenderGraphTest)

add_executable(VultanaEngine WIN32 WinMain.cpp ${SHADER_FILES})
target_link_libraries(VultanaEngine FrameworkLib)
target_include_directories(VultanaEngine PUBLIC ${PROJECT_SOURCE_DIR}/Framework)