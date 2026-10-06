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
set(WG_AUDIO_BACKEND "standard" CACHE STRING
    "Audio backend: standard or silent")
set(WG_OPL_BACKEND "nuked" CACHE STRING
    "OPL emulator for standard audio: nuked or dbopl")
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

set(WG_OPL_SOURCE)
set(WG_OPL_LIBRARY)
if(WG_AUDIO_BACKEND STREQUAL "silent")
    set(WG_OPL_SOURCE "${WG_ROOT}/src/WG_OPL_SILENT.c")
elseif(WG_AUDIO_BACKEND STREQUAL "standard")
    if(WG_OPL_BACKEND STREQUAL "nuked")
        set(WG_OPL_SOURCE "${WG_ROOT}/src/WG_OPL_NUKED.c")
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
    elseif(WG_OPL_BACKEND STREQUAL "dbopl")
        set(WG_OPL_SOURCE
            "${WG_ROOT}/src/WG_OPL_DBOPL.c"
            "${WG_ROOT}/third_party/DBOPL/dbopl.c")
    else()
        message(FATAL_ERROR "Unsupported WG_OPL_BACKEND: ${WG_OPL_BACKEND}")
    endif()
else()
    message(FATAL_ERROR "Unsupported WG_AUDIO_BACKEND: ${WG_AUDIO_BACKEND}")
endif()

set(WG_CORE_SOURCES
    "${WG_ROOT}/src/WG_ASSETS.c"
    "${WG_ROOT}/src/WG_AUDIO.c"
    "${WG_ROOT}/src/WG_CONFIG.c"
    "${WG_ROOT}/src/ID_CA.c"
    "${WG_ROOT}/src/ID_IN.c"
    "${WG_ROOT}/src/WG_DATA.c"
    "${WG_ROOT}/src/WG_FILE.c"
    "${WG_ROOT}/src/WG_FIXED.c"
    "${WG_ROOT}/src/ID_VH.c"
    "${WG_ROOT}/src/WG_GRAPHICS.c"
    "${WG_ROOT}/src/WL_GAME.c"
    "${WG_ROOT}/src/WG_MAPS.c"
    "${WG_ROOT}/src/ID_PM.c"
    "${WG_ROOT}/src/ID_SD.c"
    ${WG_OPL_SOURCE}
    "${WG_ROOT}/src/WG_PALETTE.c"
    "${WG_ROOT}/src/WG_PLATFORM.c"
    "${WG_ROOT}/src/ID_US_1.c"
    "${WG_ROOT}/src/WL_AGENT.c"
    "${WG_ROOT}/src/WL_ACT1.c"
    "${WG_ROOT}/src/WL_ACT2.c"
    "${WG_ROOT}/src/WL_PLAY.c"
    "${WG_ROOT}/src/WL_STATE.c"
    "${WG_ROOT}/src/WL_DRAW.c"
    "${WG_ROOT}/src/WL_INTER.c"
    "${WG_ROOT}/src/WL_TEXT.c"
    "${WG_ROOT}/src/WG_RENDERER.c"
    "${WG_ROOT}/src/WG_SAVE.c"
    "${WG_ROOT}/src/WG_SIGNON.c"
    "${WG_ROOT}/src/WL_SCALE.c"
    "${WG_ROOT}/src/ID_VL.c"
    "${WG_ROOT}/src/WL_MAIN.c"
    "${WG_ROOT}/src/WL_MENU.c"
    "${WG_ROOT}/src/WOLF3D.c")

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
    _CRT_SECURE_NO_WARNINGS)

if(WG_AUDIO_BACKEND STREQUAL "silent")
    target_compile_definitions(wolf3d PRIVATE WG_AUDIO_SILENT=1)
elseif(WG_OPL_BACKEND STREQUAL "dbopl")
    target_compile_definitions(wolf3d PRIVATE WG_OPL_DBOPL=1)
endif()

if(WG_OPL_LIBRARY)
    target_link_libraries(wolf3d PRIVATE ${WG_OPL_LIBRARY})
endif()

add_executable(wolf3d-public-api-consumer
    "${WG_ROOT}/tests/WOLF3D_CONSUMER.c")
target_link_libraries(wolf3d-public-api-consumer PRIVATE wolf3d)
target_include_directories(wolf3d-public-api-consumer PRIVATE
    "${WG_ROOT}/include")

if(MSVC_VERSION EQUAL 1200)
    set(WG_COMPILER_LABEL vc6)
elseif(MSVC_VERSION EQUAL 1300)
    set(WG_COMPILER_LABEL vs2002)
elseif(MSVC_VERSION EQUAL 1310)
    set(WG_COMPILER_LABEL vs2003)
elseif(MSVC_VERSION EQUAL 1400)
    set(WG_COMPILER_LABEL vs2005)
else()
    set(WG_COMPILER_LABEL msvc-legacy)
endif()

set(WG_RUNTIME_SUFFIX "")
if(NOT WG_STATIC_MSVC_RUNTIME)
    set(WG_RUNTIME_SUFFIX "-dynamic-crt")
endif()
set(WG_AUDIO_SUFFIX "")
if(WG_AUDIO_BACKEND STREQUAL "silent")
    set(WG_AUDIO_SUFFIX "-silent")
elseif(WG_OPL_BACKEND STREQUAL "dbopl")
    set(WG_AUDIO_SUFFIX "-dbopl")
endif()
set(WG_RELEASE_DIR
    "${WG_DIST_ROOT}/wolf3d-${WG_PROJECT_VERSION}-library-windows-x86-${WG_COMPILER_LABEL}${WG_RUNTIME_SUFFIX}${WG_AUDIO_SUFFIX}")

configure_file("${WG_ROOT}/packaging/LIBRARY-README.txt.in"
               "${CMAKE_CURRENT_BINARY_DIR}/LIBRARY-README.txt" @ONLY)

set(WG_RELEASE_COMMANDS
    COMMAND ${CMAKE_COMMAND} -E remove_directory "${WG_RELEASE_DIR}"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${WG_RELEASE_DIR}"
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
elseif(WG_AUDIO_BACKEND STREQUAL "standard"
       AND WG_OPL_BACKEND STREQUAL "dbopl")
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
