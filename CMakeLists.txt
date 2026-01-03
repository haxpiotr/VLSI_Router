cmake_minimum_required(VERSION 3.15)
project(HighPerformanceICRouter VERSION 1.0.0 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(EXT_LIB_LEFDEF_INCLUDE "C:/Users/haxpi/source/repos/form_files/LEF-DEF_6.0_62-p004/include")
set(EXT_LIB_LEF_DEBUG   "C:/Users/haxpi/source/repos/form_files/LEF-DEF_6.0_62-p004/lib/x64/dbg/lef.lib")
set(EXT_LIB_LEF_RELEASE "C:/Users/haxpi/source/repos/form_files/LEF-DEF_6.0_62-p004/lib/x64/opt/lef.lib")
set(EXT_LIB_DEF_DEBUG   "C:/Users/haxpi/source/repos/form_files/LEF-DEF_6.0_62-p004/lib/x64/dbg/def.lib")
set(EXT_LIB_DEF_RELEASE "C:/Users/haxpi/source/repos/form_files/LEF-DEF_6.0_62-p004/lib/x64/opt/def.lib")

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

set(LIB_SOURCES
    "DataTransformer.cpp"
    "DEFLoader.cpp"
    "LEFLoader.cpp"
    "GlobalRouter.cpp"
)

add_library(${PROJECT_NAME} STATIC ${LIB_SOURCES})

set(VCPKG_CMAKE "C:/Users/haxpi/vcpkg/scripts/buildsystems/vcpkg.cmake")
include(${VCPKG_CMAKE})

find_package(Boost REQUIRED)

if(Boost_FOUND)
    target_include_directories(${PROJECT_NAME} PUBLIC ${Boost_INCLUDE_DIRS})
endif()

target_link_libraries(${PROJECT_NAME} PUBLIC 
    LefLib 
    DefLib
)

target_include_directories(${PROJECT_NAME} PUBLIC 
    "${CMAKE_CURRENT_SOURCE_DIR}"
)

if(MSVC)
    target_compile_definitions(${PROJECT_NAME} PRIVATE _CRT_SECURE_NO_WARNINGS)
    target_compile_options(${PROJECT_NAME} PRIVATE /W4)
else()
    target_compile_options(${PROJECT_NAME} PRIVATE -Wall)
endif()

enable_testing()
find_package(GTest CONFIG REQUIRED)
add_subdirectory(Tests)