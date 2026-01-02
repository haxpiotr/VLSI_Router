add_executable(RouterTests test.cpp)

target_link_libraries(RouterTests PRIVATE 
    HighPerformanceICRouter
    GTest::gtest
    GTest::gtest_main
)

include(GoogleTest)
target_include_directories(RouterTests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/..")
gtest_discover_tests(RouterTests)

file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/Data" 
     DESTINATION "${CMAKE_CURRENT_BINARY_DIR}")