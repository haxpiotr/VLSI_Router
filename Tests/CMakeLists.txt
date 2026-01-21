add_executable(RouterTests test.cpp)

target_link_libraries(RouterTests PRIVATE 
    HighPerformanceICRouter
    GTest::gtest
    GTest::gtest_main
)

# Apply sanitizers to test executable for Debug builds
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_options(RouterTests PRIVATE -fsanitize=address -fsanitize=undefined)
    target_link_options(RouterTests PRIVATE -fsanitize=address -fsanitize=undefined)
endif()

include(GoogleTest)
target_include_directories(RouterTests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/..")
gtest_discover_tests(RouterTests)

add_custom_command(
    TARGET RouterTests POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/Data"
        "$<TARGET_FILE_DIR:RouterTests>/Data"
    COMMENT "Copying test data files"
)