cmake_minimum_required(VERSION 3.14.0)
project(photo_viewer)

if(GENERATOR_IS_MULTI_CONFIG)
    # Add "Profile" for multi configuration generator
    if(NOT "Profile" IN_LIST CMAKE_CONFIGURATION_TYPES)
        list(APPEND CMAKE_CONFIGURATION_TYPES Profile)
    endif()
else()
    # Add "Profile" for single configuration generator
    set(allowableBuildTypes Debug Release Profile)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY
    STRINGS "${allowableBuildTypes}")
    if(NOT CMAKE_BUILD_TYPE)
        set(CMAKE_BUILD_TYPE Debug CACHE STRING "" FORCE)
    elseif(NOT CMAKE_BUILD_TYPE IN_LIST allowableBuildTypes)
        message(FATAL_ERROR "Invalid build type: ${CMAKE_BUILD_TYPE}")
    endif()
endif()

#set(CMAKE_C_FLAGS_PROFILE             "-p -g -O2" CACHE STRING "")
#set(CMAKE_CXX_FLAGS_PROFILE           "-p -g -O2" CACHE STRING "")
#set(CMAKE_EXE_LINKER_FLAGS_PROFILE    "-p -g -O2" CACHE STRING "")
#set(CMAKE_SHARED_LINKER_FLAGS_PROFILE "-p -g -O2" CACHE STRING "")
#set(CMAKE_STATIC_LINKER_FLAGS_PROFILE "-p -g -O2" CACHE STRING "")
#set(CMAKE_MODULE_LINKER_FLAGS_PROFILE "-p -g -O2" CACHE STRING "")

if (MSVC)
    # warning level 4 and all warnings as errors
    add_compile_options(/W4 /WX)
else()
    # lots of warnings and all warnings as errors
    add_compile_options(-Wall -Wextra -Wpedantic -Werror)
endif()

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    if (CMAKE_BUILD_TYPE STREQUAL "Debug")
        string(APPEND CMAKE_CXX_FLAGS " -fno-omit-frame-pointer")
    endif()
    if (CMAKE_BUILD_TYPE STREQUAL "Profile")
        # following command set enables profiling
        string(APPEND CMAKE_CXX_FLAGS " -pg")
        string(APPEND CMAKE_EXE_LINKER_FLAGS " -pg")
        string(APPEND CMAKE_SHARED_LINKER_FLAGS " -pg")
    endif()
endif()

add_subdirectory(src)
add_subdirectory(app)
if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_subdirectory(tests)
endif()