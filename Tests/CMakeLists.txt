include_directories(${PROJECT_SOURCE_DIR}/Framework)

add_executable(VultanaEngine WIN32 WinMain.cpp ${SHADER_FILES})
target_link_libraries(VultanaEngine FrameworkLib)
target_include_directories(VultanaEngine PUBLIC ${PROJECT_SOURCE_DIR}/Framework)

# --- GTest ---
find_package(GTest CONFIG REQUIRED)

add_executable(UnitTests MainTest.cpp EditCommandTest.cpp ${SHADER_FILES})
target_link_libraries(UnitTests FrameworkLib GTest::gtest)
target_include_directories(UnitTests PUBLIC ${PROJECT_SOURCE_DIR}/Framework)

add_custom_target(RunUnitTests COMMAND UnitTests)
add_dependencies(RunUnitTests UnitTests)
add_test(NAME UnitTests COMMAND UnitTests WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}/Binary")