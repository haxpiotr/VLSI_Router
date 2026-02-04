cmake_minimum_required(VERSION 3.15)
project(HighPerformanceICRouter VERSION 1.0.0 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(OpenMP)
if (OPENMP_FOUND)
    message(STATUS "OpenMP found")
    set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
    set (CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${OpenMP_EXE_LINKER_FLAGS}")
endif()

set(EXT_LIB_DEF "/home/piotr/repos/VLSI_Router/LefDef/def_6.0_62-p004/def")
set(EXT_LIB_DEF_INCLUDE_DIR "${EXT_LIB_DEF}/include")
set(EXT_LIB_DEF_LIB_DIR "${EXT_LIB_DEF}/lib")

set(EXT_LIB_LEF "/home/piotr/repos/VLSI_Router/LefDef/lef_6.0_62-p004/lef")
set(EXT_LIB_LEF_INCLUDE_DIR "/home/piotr/repos/VLSI_Router/LefDef/lef_6.0_62-p004/lef/include")
set(EXT_LIB_LEF_LIB_DIR "/home/piotr/repos/VLSI_Router/LefDef/lef_6.0_62-p004/lef/lib")

set(EXT_LIB_DEF_DEBUG   ${EXT_LIB_DEF_LIB_DIR}/libdef.a)
set(EXT_LIB_DEF_RELEASE  ${EXT_LIB_DEF_LIB_DIR}/libdef.a)

set(EXT_LIB_LEF_DEBUG   ${EXT_LIB_LEF_LIB_DIR}/liblef.a)
set(EXT_LIB_LEF_RELEASE  ${EXT_LIB_LEF_LIB_DIR}/liblef.a)

add_library(DefLib STATIC IMPORTED GLOBAL)
set_target_properties(DefLib PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "${EXT_LIB_DEF_INCLUDE_DIR}"
    IMPORTED_LOCATION "${EXT_LIB_DEF_RELEASE}"
)

add_library(LefLib STATIC IMPORTED GLOBAL)
set_target_properties(LefLib PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "${EXT_LIB_LEF_INCLUDE_DIR}"
    IMPORTED_LOCATION "${EXT_LIB_LEF_RELEASE}"
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
    "HeatmapExporter.hpp"
    "NetSolution.hpp"
    "SeqSA.hpp"
    "ParSA.hpp"
    "SpacePartitionedSA.hpp"
    "SpacePartitionedGPUSA.hpp"
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
)

add_library(${PROJECT_NAME} STATIC ${LIB_SOURCES})

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

target_compile_options(${PROJECT_NAME} PRIVATE -Werror -Wextra -Wall -fopenmp-simd)

# Set optimization levels based on build type
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_options(${PROJECT_NAME} PRIVATE -O0 -g -fsanitize=address -fsanitize=undefined)
    target_link_options(${PROJECT_NAME} PRIVATE -fsanitize=address -fsanitize=undefined)
    message(STATUS "Debug build: optimizations disabled, AddressSanitizer enabled")
else()
    target_compile_options(${PROJECT_NAME} PRIVATE -O3)
    message(STATUS "Release build: optimizations enabled (-O3)")
endif()


include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/heads/main.zip
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)
enable_testing()
add_subdirectory(Tests)
