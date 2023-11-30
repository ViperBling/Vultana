include_directories(${PROJECT_SOURCE_DIR}/Framework)

add_executable(FirstTest FirstTest.cpp)
target_link_libraries(FirstTest FrameworkLib)

target_include_directories(FirstTest PUBLIC ${PROJECT_SOURCE_DIR}/Framework)