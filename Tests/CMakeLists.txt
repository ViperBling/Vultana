add_executable(FirstTest FirstTest.cpp)
target_link_libraries(FirstTest PUBLIC Utilities)
target_include_directories(FirstTest PUBLIC ${CMAKE_SOURCE_DIR}/Utilities)