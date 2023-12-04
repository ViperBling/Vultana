include_directories(${PROJECT_SOURCE_DIR}/Framework)

add_executable(EngineTest EngineTest.cpp)
target_link_libraries(EngineTest FrameworkLib)

target_include_directories(EngineTest PUBLIC ${PROJECT_SOURCE_DIR}/Framework)