add_executable(RouterTests test.cpp)

target_link_libraries(RouterTests PRIVATE 
    HighPerformanceICRouter
    GTest::gtest
    GTest::gtest_main
)

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