cmake_minimum_required(VERSION 3.15)
project(HighPerformanceICRouter VERSION 1.0.0 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

#add_subdirectory(flute)

find_package(OpenMP)
if (OPENMP_FOUND)
    message(STATUS "OpenMP found")
    set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
    set (CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${OpenMP_EXE_LINKER_FLAGS}")
endif()


set(EXT_LIB_DEF_INCLUDE_DIR $ENV{EXT_LIB_DEF}/include)

set(EXT_LIB_LEF_INCLUDE_DIR $ENV{EXT_LIB_LEF}/include)

set(EXT_LIB_DEF_DEBUG $ENV{EXT_LIB_LEFDEF_DIR}/dbg/def.lib)
set(EXT_LIB_DEF_RELEASE $ENV{EXT_LIB_LEFDEF_DIR}/opt/def.lib)

set(EXT_LIB_LEF_DEBUG $ENV{EXT_LIB_LEFDEF_DIR}/dbg/lef.lib)
set(EXT_LIB_LEF_RELEASE $ENV{EXT_LIB_LEFDEF_DIR}/opt/lef.lib)

add_library(DefLib STATIC IMPORTED GLOBAL)
set_target_properties(DefLib PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "${EXT_LIB_DEF_INCLUDE_DIR}"
    IMPORTED_LOCATION_RELEASE "${EXT_LIB_DEF_RELEASE}"
    IMPORTED_LOCATION_DEBUG "${EXT_LIB_DEF_DEBUG}"
)

add_library(LefLib STATIC IMPORTED GLOBAL)
set_target_properties(LefLib PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "${EXT_LIB_LEF_INCLUDE_DIR}"
    IMPORTED_LOCATION_RELEASE "${EXT_LIB_LEF_RELEASE}"
    IMPORTED_LOCATION_DEBUG "${EXT_LIB_LEF_DEBUG}"
)

set(LIB_HEADERS
    "DataTransformer.hpp"
    "DEFLoader.hpp"
    "LEFLoader.hpp"
    "GlobalRouter.hpp"
    "LEFData.hpp"
    "DEFData.hpp"
    "ISpecializedRouter.hxx"
    "IOptimizationSolver.hxx"
    "DoglegRouter.hpp"
    "NetSolution.hpp"
    "SeqSA.hpp"
    "ParSA.hpp"
    "SpacePartitionedSA.hpp"
    "SpacePartitionedGPUSA.hpp"
    "MinimumSpanningTree.hpp"
    "GeometryTypes.hpp"
    "RoutingDataTypes.hpp"
    "OptimizationKernels.hpp"
    "RNDPerTStepSpacePartitionedGPUSA.hpp"
    "GeneticAlgorithm.hpp"
)

set(LIB_SOURCES
    "DataTransformer.cpp"
    "DEFLoader.cpp"
    "LEFLoader.cpp"
    "GlobalRouter.cpp"
    "DoglegRouter.cpp"
    "NetSolution.cpp"
    "SeqSA.cpp"
    "ParSA.cpp"
    "SpacePartitionedSA.cpp"
    "SpacePartitionedGPUSA.cpp"
    "MinimumSpanningTree.cpp"
    "OptimizationKernels.cpp"
    "RNDPerTStepSpacePartitionedGPUSA.cpp"
    "GeneticAlgorithm.cpp"
)

add_library(${PROJECT_NAME} STATIC ${LIB_SOURCES})

find_package(OpenCL REQUIRED)
if(OpenCL_FOUND)
    message(STATUS "OpenCL found: ${OpenCL_INCLUDE_DIRS}")
    include_directories(${OpenCL_INCLUDE_DIRS})
    target_link_libraries(${PROJECT_NAME} PUBLIC OpenCL::OpenCL)
else()
    message(FATAL_ERROR "OpenCL not found")
endif()


target_link_libraries(${PROJECT_NAME} PUBLIC 
    LefLib 
    DefLib
    OpenMP::OpenMP_CXX
)

include(CheckIPOSupported)
check_ipo_supported(RESULT supported OUTPUT error)

if(supported)
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
else()
    message(STATUS "LTO not supported: ${error}")
endif()

target_include_directories(${PROJECT_NAME} PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# Set optimization levels based on build type
if(NOT MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE -Werror -Wextra -Wall -fopenmp-simd)
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        target_compile_options(${PROJECT_NAME} PRIVATE -O0 -g -fsanitize=address -fsanitize=undefined)
        target_link_options(${PROJECT_NAME} PRIVATE -fsanitize=address -fsanitize=undefined)
        message(STATUS "NOT MSVC Debug build: optimizations disabled, AddressSanitizer enabled")
    else()
        target_compile_options(${PROJECT_NAME} PRIVATE -O3)
        message(STATUS "NOT MSVC Release build: optimizations enabled (-O3)")
    endif()
else()
    target_compile_definitions(${PROJECT_NAME} PRIVATE _CRT_SECURE_NO_WARNINGS NOMINMAX BOOST_COMPUTE_USE_CPP11)
    target_compile_options(${PROJECT_NAME} PRIVATE /W3 /openmp:experimental)
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        target_compile_options(${PROJECT_NAME} PRIVATE /Od /Zi)
        message(STATUS "Debug build: optimizations disabled, debug symbols enabled")
    else()
        target_compile_options(${PROJECT_NAME} PRIVATE /O2)
        message(STATUS "Release build: optimizations enabled (/O2)")
    endif()
endif()

find_package(GTest REQUIRED)
enable_testing()
add_subdirectory(Tests)