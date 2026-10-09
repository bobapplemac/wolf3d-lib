cmake_minimum_required(VERSION 3.5)

# This intentionally separate build definition keeps support for the Visual
# Studio 6 through 2005 generators out of the modern root CMake project.
project(wolf3d_legacy C)

get_filename_component(WG_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../.." ABSOLUTE)
file(READ "${WG_ROOT}/VERSION" WG_PROJECT_VERSION)
string(STRIP "${WG_PROJECT_VERSION}" WG_PROJECT_VERSION)
set(PROJECT_VERSION "${WG_PROJECT_VERSION}")

option(WG_STATIC_MSVC_RUNTIME
       "Link the legacy MSVC runtime statically" ON)
option(WG_ENABLE_OPL_NUKED "Compile the Nuked OPL3 runtime driver" ON)
option(WG_ENABLE_OPL_DBOPL "Compile the DBOPL runtime driver" ON)
option(WG_ENABLE_OPL_SILENT "Compile the timing-preserving silent driver" ON)
set(WG_DEFAULT_OPL_DRIVER "nuked" CACHE STRING
    "Default runtime OPL driver: nuked, dbopl, or silent")
set(WG_DEFAULT_SAMPLE_RATE "48000" CACHE STRING
    "Preferred PCM sample rate")
set(WG_DIST_ROOT "${WG_ROOT}/dist" CACHE PATH
    "Directory used by legacy release staging targets")

if(NOT MSVC)
    message(FATAL_ERROR "The legacy build is only for Microsoft Visual C++")
endif()

if(MSVC_VERSION GREATER 1400)
    message(FATAL_ERROR
        "Use the root CMake project for Visual Studio 2008 and newer")
endif()

if(WG_STATIC_MSVC_RUNTIME)
    foreach(WG_FLAG_VARIABLE
            CMAKE_C_FLAGS CMAKE_C_FLAGS_DEBUG CMAKE_C_FLAGS_RELEASE
            CMAKE_C_FLAGS_MINSIZEREL CMAKE_C_FLAGS_RELWITHDEBINFO)
        if(DEFINED ${WG_FLAG_VARIABLE})
            string(REPLACE "/MDd" "/MTd" ${WG_FLAG_VARIABLE}
                   "${${WG_FLAG_VARIABLE}}")
            string(REPLACE "/MD" "/MT" ${WG_FLAG_VARIABLE}
                   "${${WG_FLAG_VARIABLE}}")
        endif()
    endforeach()
endif()

set(WG_OPL_SOURCE "${WG_ROOT}/src/WG_OPL.c")
set(WG_OPL_LIBRARY)
if(WG_ENABLE_OPL_NUKED)
        list(APPEND WG_OPL_SOURCE "${WG_ROOT}/src/WG_OPL_NUKED.c")
        add_library(nuked-opl3 SHARED
            "${WG_ROOT}/third_party/Nuked-OPL3/opl3.c"
            "${CMAKE_CURRENT_SOURCE_DIR}/Nuked-OPL3.def")
        set_target_properties(nuked-opl3 PROPERTIES
            OUTPUT_NAME "Nuked-OPL3")
        target_include_directories(nuked-opl3 PUBLIC
            "${WG_ROOT}/compat/msvc"
            "${WG_ROOT}/third_party/Nuked-OPL3")
        if(MSVC_VERSION LESS 1300)
            # VC6 does not emit an externally linkable copy of Nuked's one
            # public inline function. Compile it as an ordinary function.
            target_compile_definitions(nuked-opl3 PRIVATE inline=)
        else()
            target_compile_definitions(nuked-opl3 PRIVATE inline=__inline)
        endif()
        set(WG_OPL_LIBRARY nuked-opl3)
endif()
if(WG_ENABLE_OPL_DBOPL)
        list(APPEND WG_OPL_SOURCE
            "${WG_ROOT}/src/WG_OPL_DBOPL.c"
            "${WG_ROOT}/third_party/DBOPL/dbopl.c")
endif()
if(WG_ENABLE_OPL_SILENT)
    list(APPEND WG_OPL_SOURCE "${WG_ROOT}/src/WG_OPL_SILENT.c")
endif()
if(NOT WG_ENABLE_OPL_NUKED AND NOT WG_ENABLE_OPL_DBOPL
   AND NOT WG_ENABLE_OPL_SILENT)
    message(FATAL_ERROR "At least one OPL driver must be enabled")
endif()
string(TOUPPER "${WG_DEFAULT_OPL_DRIVER}" WG_DEFAULT_OPL_DRIVER_UPPER)
if(NOT DEFINED WG_ENABLE_OPL_${WG_DEFAULT_OPL_DRIVER_UPPER}
   OR NOT WG_ENABLE_OPL_${WG_DEFAULT_OPL_DRIVER_UPPER})
    message(FATAL_ERROR "The default OPL driver must be enabled")
endif()

file(STRINGS "${WG_ROOT}/cmake/WGCoreSources.txt" WG_CORE_RELATIVE_SOURCES)
set(WG_CORE_SOURCES "")
foreach(WG_SOURCE ${WG_CORE_RELATIVE_SOURCES})
    list(APPEND WG_CORE_SOURCES "${WG_ROOT}/${WG_SOURCE}")
endforeach()
list(APPEND WG_CORE_SOURCES ${WG_OPL_SOURCE})

add_library(wolf3d SHARED ${WG_CORE_SOURCES})
set_target_properties(wolf3d PROPERTIES OUTPUT_NAME "wolf3d")
target_include_directories(wolf3d
    PUBLIC "${WG_ROOT}/include"
    PRIVATE
        "${WG_ROOT}/compat/msvc"
        "${WG_ROOT}/src"
        "${WG_ROOT}/third_party/DBOPL")
target_compile_definitions(wolf3d PRIVATE
    WOLF3D_BUILD
    inline=__inline
    _CRT_SECURE_NO_WARNINGS
    WG_DEFAULT_OPL_DRIVER="${WG_DEFAULT_OPL_DRIVER}"
    WG_DEFAULT_SAMPLE_RATE=${WG_DEFAULT_SAMPLE_RATE}U)
if(WG_ENABLE_OPL_NUKED)
    target_compile_definitions(wolf3d PRIVATE WG_OPL_ENABLE_NUKED=1)
endif()
if(WG_ENABLE_OPL_DBOPL)
    target_compile_definitions(wolf3d PRIVATE WG_OPL_ENABLE_DBOPL=1)
endif()
if(WG_ENABLE_OPL_SILENT)
    target_compile_definitions(wolf3d PRIVATE WG_OPL_ENABLE_SILENT=1)
endif()

if(WG_OPL_LIBRARY)
    target_link_libraries(wolf3d PRIVATE ${WG_OPL_LIBRARY})
endif()

add_executable(wolf3d-public-api-consumer
    "${WG_ROOT}/tests/WOLF3D_CONSUMER.c")
target_link_libraries(wolf3d-public-api-consumer PRIVATE wolf3d)
target_include_directories(wolf3d-public-api-consumer PRIVATE
    "${WG_ROOT}/include")

include("${WG_ROOT}/cmake/WGDistribution.cmake")
wg_distribution(WG_RELEASE_DIR wolf3d-lib "${WG_PROJECT_VERSION}" "" "${WG_ROOT}" "${WG_DIST_ROOT}")

configure_file("${WG_ROOT}/packaging/LIBRARY-README.txt.in"
               "${CMAKE_CURRENT_BINARY_DIR}/LIBRARY-README.txt" @ONLY)

set(WG_RELEASE_COMMANDS
    COMMAND ${CMAKE_COMMAND} -E remove_directory "${WG_RELEASE_DIR}"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${WG_RELEASE_DIR}"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${WG_RELEASE_DIR_INFO}" "${WG_RELEASE_DIR}/BUILD-INFO.txt"
        COMMAND ${CMAKE_COMMAND} "-DINFO=${WG_RELEASE_DIR}/BUILD-INFO.txt"
            "-DROOT=${WG_ROOT}" "-DENGINE=${WG_ROOT}"
            "-DCACHE=${CMAKE_BINARY_DIR}/CMakeCache.txt"
            "-DSETTINGS=${CMAKE_CURRENT_BINARY_DIR}/distribution-target-settings.txt"
            -P "${WG_ROOT}/cmake/WGStampBuildInfo.cmake"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${WG_RELEASE_DIR}/LICENSES"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "$<TARGET_FILE:wolf3d>" "${WG_RELEASE_DIR}/wolf3d.dll"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "$<TARGET_LINKER_FILE:wolf3d>" "${WG_RELEASE_DIR}/wolf3d.lib"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${WG_ROOT}/include/WOLF3D.h" "${WG_RELEASE_DIR}/WOLF3D.h"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${WG_ROOT}/include/WOLF3D_STDINT.h"
        "${WG_RELEASE_DIR}/WOLF3D_STDINT.h"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${WG_ROOT}/LICENSE" "${WG_RELEASE_DIR}/LICENSE.txt"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${WG_ROOT}/THIRD_PARTY.md"
        "${WG_RELEASE_DIR}/THIRD_PARTY_NOTICES.txt"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_BINARY_DIR}/LIBRARY-README.txt"
        "${WG_RELEASE_DIR}/README.txt")

set(WG_RELEASE_DEPENDS wolf3d)
if(WG_OPL_LIBRARY)
    list(APPEND WG_RELEASE_COMMANDS
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "$<TARGET_FILE:nuked-opl3>"
            "${WG_RELEASE_DIR}/Nuked-OPL3.dll"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${WG_ROOT}/third_party/Nuked-OPL3/LICENSE"
            "${WG_RELEASE_DIR}/LICENSES/Nuked-OPL3-LGPL-2.1.txt")
    list(APPEND WG_RELEASE_DEPENDS nuked-opl3)
endif()
if(WG_ENABLE_OPL_DBOPL)
    list(APPEND WG_RELEASE_COMMANDS
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${WG_ROOT}/third_party/DBOPL/README.wolf3d-lib.md"
            "${WG_RELEASE_DIR}/README.DBOPL.txt")
endif()

add_custom_target(library_release
    ${WG_RELEASE_COMMANDS}
    DEPENDS ${WG_RELEASE_DEPENDS})

if(MSVC_VERSION LESS 1300)
    target_compile_options(wolf3d PRIVATE /W3)
    if(TARGET nuked-opl3)
        target_compile_options(nuked-opl3 PRIVATE /W3)
    endif()
else()
    target_compile_options(wolf3d PRIVATE /W4)
    if(TARGET nuked-opl3)
        target_compile_options(nuked-opl3 PRIVATE /W4)
    endif()
endif()

wg_distribution_target_info("${CMAKE_CURRENT_SOURCE_DIR}")
