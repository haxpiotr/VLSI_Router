cmake_minimum_required(VERSION 3.15)
project(HighPerformanceICRouter VERSION 1.0.0 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(EXT_LIB_LEFDEF_INCLUDE $ENV{LEFDEF_PATH}/include)

if(MSVC)
    set(EXT_LIB_LEF_DEBUG   $ENV{LEFDEF_PATH}/lib/x64/dbg/lef.lib)
    set(EXT_LIB_LEF_RELEASE $ENV{LEFDEF_PATH}/lib/x64/opt/lef.lib)
    set(EXT_LIB_DEF_DEBUG   $ENV{LEFDEF_PATH}/lib/x64/dbg/def.lib)
    set(EXT_LIB_DEF_RELEASE $ENV{LEFDEF_PATH}/lib/x64/opt/def.lib)
else()
    set(EXT_LIB_LEF_DEBUG   $ENV{LEFDEF_PATH}/lib/x64/dbg/liblef_debug.a)
    set(EXT_LIB_LEF_RELEASE $ENV{LEFDEF_PATH}/lib/x64/opt/liblef.a)
    set(EXT_LIB_DEF_DEBUG   $ENV{LEFDEF_PATH}/lib/x64/dbg/libdef_debug.a)
    set(EXT_LIB_DEF_RELEASE $ENV{LEFDEF_PATH}/lib/x64/opt/libdef.a)
endif()

add_library(LefLib STATIC IMPORTED)
set_target_properties(LefLib PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "${EXT_LIB_LEFDEF_INCLUDE}"
    IMPORTED_LOCATION_DEBUG   "${EXT_LIB_LEF_DEBUG}"
    IMPORTED_LOCATION_RELEASE "${EXT_LIB_LEF_RELEASE}"
)

add_library(DefLib STATIC IMPORTED)
set_target_properties(DefLib PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "${EXT_LIB_LEFDEF_INCLUDE}"
    IMPORTED_LOCATION_DEBUG   "${EXT_LIB_DEF_DEBUG}"
    IMPORTED_LOCATION_RELEASE "${EXT_LIB_DEF_RELEASE}"
)

set(LIB_HEADERS
    "DataTransformer.hpp"
    "DEFLoader.hpp"
    "LEFLoader.hpp"
    "GlobalRouter.hpp"
    "LEFData.hpp"
    "DEFData.hpp"
)

set(LIB_SOURCES
    "DataTransformer.cpp"
    "DEFLoader.cpp"
    "LEFLoader.cpp"
    "GlobalRouter.cpp"
)

add_library(${PROJECT_NAME} STATIC ${LIB_SOURCES})

if(MSVC)
    set(VCPKG_CMAKE $ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake)
    include(${VCPKG_CMAKE})
    find_package(Boost REQUIRED)
endif()



if(Boost_FOUND)
    target_include_directories(${PROJECT_NAME} PUBLIC ${Boost_INCLUDE_DIRS})
endif()

target_link_libraries(${PROJECT_NAME} PUBLIC 
    LefLib 
    DefLib
)

target_include_directories(${PROJECT_NAME} PUBLIC 
    LIB_HEADERS
)

if(MSVC)
    target_compile_definitions(${PROJECT_NAME} PRIVATE _CRT_SECURE_NO_WARNINGS)
    target_compile_options(${PROJECT_NAME} PRIVATE /W3 /openmp:experimental)
    if(CMAKE_BUILD_TYPE STREQUAL "Release")
        message(STATUS "Release Mode")
        add_definitions(/O2)
    else()
        message(STATUS "Debug Mode")
    endif()
else()
    target_compile_options(${PROJECT_NAME} PRIVATE -Werror -Wextra -Wall -fopenmp)
endif()

enable_testing()
find_package(GTest CONFIG REQUIRED)
add_subdirectory(Tests)
