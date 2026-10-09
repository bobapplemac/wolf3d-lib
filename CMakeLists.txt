cmake_minimum_required(VERSION 3.16)

if(POLICY CMP0091)
    cmake_policy(SET CMP0091 NEW)
endif()

include(cmake/WGVersion.cmake)
project(wolf3d VERSION "${WG_PROJECT_VERSION}" LANGUAGES C)

set(WG_MSVC_COMPAT_INCLUDE "")
if(MSVC AND MSVC_VERSION LESS 1800)
    set(WG_MSVC_COMPAT_INCLUDE "${CMAKE_CURRENT_SOURCE_DIR}/compat/msvc")
endif()

if(NOT PROJECT_VERSION STREQUAL WG_PROJECT_VERSION)
    message(FATAL_ERROR
        "Configured project version does not match the authoritative VERSION file")
endif()

include(CTest)

set(WG_ORIGINAL_SOURCE_PATH "" CACHE PATH
    "Optional path to the original Wolf3D WOLFSRC directory")
set(WG_CHOCOLATE_SOURCE_PATH "" CACHE PATH
    "Optional path to the Chocolate-Wolfenstein-3D source directory")

find_package(Python3 COMPONENTS Interpreter QUIET)
if(Python3_Interpreter_FOUND
   AND WG_ORIGINAL_SOURCE_PATH
   AND WG_CHOCOLATE_SOURCE_PATH)
    add_custom_target(callgraph-audit
        COMMAND ${Python3_EXECUTABLE}
            ${CMAKE_SOURCE_DIR}/tools/CALLGRAPH_AUDIT.py
            --original ${WG_ORIGINAL_SOURCE_PATH}
            --bridge ${WG_CHOCOLATE_SOURCE_PATH}
            --report ${CMAKE_BINARY_DIR}/callgraph-audit.md
            --json ${CMAKE_BINARY_DIR}/callgraph-audit.json
            --dot-dir ${CMAKE_BINARY_DIR}/callgraphs
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        COMMENT "Comparing original, Chocolate, and wolf3dgeneric call graphs"
        VERBATIM)
endif()

option(WG_BUILD_HEADLESS
    "Build the internal deterministic validation host and tests" ON)
option(WG_WARNINGS_AS_ERRORS "Treat project warnings as errors" ON)
option(WG_STATIC_MSVC_RUNTIME
    "Link the MSVC runtime statically for self-contained Windows releases" ON)
option(WG_STATIC_GNU_RUNTIME
    "Link the GCC support runtime statically in MinGW artifacts" ON)
option(WG_ENABLE_OPL_NUKED "Compile the Nuked OPL3 runtime driver" ON)
option(WG_ENABLE_OPL_DBOPL "Compile the DBOPL runtime driver" ON)
option(WG_ENABLE_OPL_SILENT "Compile the timing-preserving silent driver" ON)
set(WG_DEFAULT_OPL_DRIVER "nuked" CACHE STRING
    "Default runtime OPL driver: nuked, dbopl, or silent")
set_property(CACHE WG_DEFAULT_OPL_DRIVER PROPERTY STRINGS nuked dbopl silent)
set(WG_DEFAULT_SAMPLE_RATE "48000" CACHE STRING
    "Preferred PCM sample rate; the host may negotiate another rate")
set(WG_LINUX_LIBC "" CACHE STRING
    "Optional Linux package ABI label, such as glibc or musl")
set_property(CACHE WG_LINUX_LIBC PROPERTY STRINGS "" glibc musl)
set(WG_COMPILER_LABEL "" CACHE STRING
    "Optional compiler/toolset label appended to staged package names")
set(WG_DIST_ROOT "${CMAKE_SOURCE_DIR}/dist" CACHE PATH
    "Directory used by release staging targets")

if(MSVC AND WG_STATIC_MSVC_RUNTIME)
    set(CMAKE_MSVC_RUNTIME_LIBRARY
        "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

set(WG_OPL_SOURCES src/WG_OPL.c)
set(WG_OPL_LIBRARY_TARGETS "")
if(WG_ENABLE_OPL_NUKED)
        list(APPEND WG_OPL_SOURCES src/WG_OPL_NUKED.c)
        # Keep the LGPL component independently replaceable on every platform.
        add_library(nuked-opl3 SHARED third_party/Nuked-OPL3/opl3.c)
        set_target_properties(nuked-opl3 PROPERTIES
            OUTPUT_NAME "Nuked-OPL3"
            WINDOWS_EXPORT_ALL_SYMBOLS ON
        )
        if(MINGW)
            set_target_properties(nuked-opl3 PROPERTIES PREFIX "")
            if(WG_STATIC_GNU_RUNTIME)
                target_link_options(nuked-opl3 PRIVATE -static-libgcc)
            endif()
        endif()
        if(MSVC AND MSVC_VERSION LESS 1900)
            target_compile_definitions(nuked-opl3 PRIVATE inline=__inline)
        endif()
        target_include_directories(nuked-opl3 PUBLIC third_party/Nuked-OPL3)
        if(WG_MSVC_COMPAT_INCLUDE)
            target_include_directories(nuked-opl3 PRIVATE
                "${WG_MSVC_COMPAT_INCLUDE}")
        endif()
        target_compile_features(nuked-opl3 PUBLIC c_std_99)
        list(APPEND WG_OPL_LIBRARY_TARGETS nuked-opl3)
endif()
if(WG_ENABLE_OPL_DBOPL)
        list(APPEND WG_OPL_SOURCES
            src/WG_OPL_DBOPL.c
            third_party/DBOPL/dbopl.c
        )
        # Preserve the imported implementation unchanged. Its intentional
        # fixed-width truncations trigger host-specific conversion warnings.
        if(MSVC)
            set_source_files_properties(third_party/DBOPL/dbopl.c
                PROPERTIES COMPILE_OPTIONS "/W0")
        else()
            set_source_files_properties(third_party/DBOPL/dbopl.c
                PROPERTIES COMPILE_OPTIONS "-w")
        endif()
endif()
if(WG_ENABLE_OPL_SILENT)
    list(APPEND WG_OPL_SOURCES src/WG_OPL_SILENT.c)
endif()
if(NOT WG_ENABLE_OPL_NUKED AND NOT WG_ENABLE_OPL_DBOPL
   AND NOT WG_ENABLE_OPL_SILENT)
    message(FATAL_ERROR "At least one OPL driver must be enabled")
endif()
string(TOUPPER "${WG_DEFAULT_OPL_DRIVER}" WG_DEFAULT_OPL_DRIVER_UPPER)
if(NOT DEFINED WG_ENABLE_OPL_${WG_DEFAULT_OPL_DRIVER_UPPER}
   OR NOT WG_ENABLE_OPL_${WG_DEFAULT_OPL_DRIVER_UPPER})
    message(FATAL_ERROR
        "WG_DEFAULT_OPL_DRIVER '${WG_DEFAULT_OPL_DRIVER}' is not enabled")
endif()
if(NOT WG_DEFAULT_SAMPLE_RATE MATCHES "^[0-9]+$"
   OR WG_DEFAULT_SAMPLE_RATE LESS 8000
   OR WG_DEFAULT_SAMPLE_RATE GREATER 192000)
    message(FATAL_ERROR "WG_DEFAULT_SAMPLE_RATE must be between 8000 and 192000")
endif()

configure_file(packaging/LIBRARY-README.txt.in
               ${CMAKE_CURRENT_BINARY_DIR}/LIBRARY-README.txt @ONLY)

file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/cmake/WGCoreSources.txt"
     WG_CORE_BASE_SOURCES)
set(WG_CORE_SOURCES ${WG_CORE_BASE_SOURCES} ${WG_OPL_SOURCES})

add_library(wolf3d SHARED ${WG_CORE_SOURCES})
add_library(wolf3d::wolf3d ALIAS wolf3d)
set_target_properties(wolf3d PROPERTIES
    OUTPUT_NAME "wolf3d"
    VERSION "${PROJECT_VERSION}"
    SOVERSION "1"
    C_VISIBILITY_PRESET hidden
    BUILD_RPATH_USE_ORIGIN ON
)
if(MINGW)
    set_target_properties(wolf3d PROPERTIES PREFIX "")
endif()
target_compile_definitions(wolf3d PRIVATE WOLF3D_BUILD)

if(WG_BUILD_HEADLESS)
    add_library(wolf3d-internal STATIC ${WG_CORE_SOURCES})
    target_compile_definitions(wolf3d-internal
        PUBLIC WOLF3D_STATIC)
endif()

function(wg_configure_core target)
    if(WG_OPL_LIBRARY_TARGETS)
        target_link_libraries(${target} PRIVATE ${WG_OPL_LIBRARY_TARGETS})
    endif()

    if(WG_ENABLE_OPL_NUKED)
        target_compile_definitions(${target} PRIVATE WG_OPL_ENABLE_NUKED=1)
    endif()
    if(WG_ENABLE_OPL_DBOPL)
        target_compile_definitions(${target} PRIVATE WG_OPL_ENABLE_DBOPL=1)
    endif()
    if(WG_ENABLE_OPL_SILENT)
        target_compile_definitions(${target} PRIVATE WG_OPL_ENABLE_SILENT=1)
    endif()
    target_compile_definitions(${target} PRIVATE
        WG_DEFAULT_OPL_DRIVER="${WG_DEFAULT_OPL_DRIVER}"
        WG_DEFAULT_SAMPLE_RATE=${WG_DEFAULT_SAMPLE_RATE}U)

    target_include_directories(${target}
        PUBLIC include
        PRIVATE src third_party/DBOPL
    )
    if(WG_MSVC_COMPAT_INCLUDE)
        target_include_directories(${target} PRIVATE
            "${WG_MSVC_COMPAT_INCLUDE}")
    endif()

    target_compile_features(${target} PUBLIC c_std_99)

    if(NOT MSVC)
        target_link_libraries(${target} PRIVATE m)
    endif()

    if(MINGW AND WG_STATIC_GNU_RUNTIME)
        target_link_options(${target} PRIVATE -static-libgcc)
    endif()

    if(MSVC)
        target_compile_definitions(${target} PRIVATE _CRT_SECURE_NO_WARNINGS)
        target_compile_options(${target} PRIVATE /W4)
        if(MSVC_VERSION LESS 1900)
            target_compile_definitions(${target} PRIVATE inline=__inline)
        endif()
        if(MSVC_VERSION GREATER_EQUAL 1910)
            target_compile_options(${target} PRIVATE /permissive-)
        endif()
        if(WG_WARNINGS_AS_ERRORS)
            target_compile_options(${target} PRIVATE /WX)
        endif()
    else()
        target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic)
        if(WG_WARNINGS_AS_ERRORS)
            target_compile_options(${target} PRIVATE -Werror)
        endif()
    endif()
endfunction()

wg_configure_core(wolf3d)
if(WG_BUILD_HEADLESS)
    wg_configure_core(wolf3d-internal)
endif()

include(cmake/WGDistribution.cmake)
wg_distribution(WG_LIBRARY_RELEASE_DIR wolf3d-lib "${PROJECT_VERSION}" "" "${CMAKE_CURRENT_SOURCE_DIR}" "${WG_DIST_ROOT}")
set(WG_RELEASE_ARCH "${WG_LIBRARY_RELEASE_DIR_ARCH}")

set(WG_LIBRARY_RELEASE_COMMANDS
    COMMAND ${CMAKE_COMMAND} -E remove_directory
        "${WG_LIBRARY_RELEASE_DIR}"
    COMMAND ${CMAKE_COMMAND} -E make_directory
        "${WG_LIBRARY_RELEASE_DIR}"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${WG_LIBRARY_RELEASE_DIR_INFO}" "${WG_LIBRARY_RELEASE_DIR}/BUILD-INFO.txt"
        COMMAND ${CMAKE_COMMAND} "-DINFO=${WG_LIBRARY_RELEASE_DIR}/BUILD-INFO.txt"
            "-DROOT=${CMAKE_CURRENT_SOURCE_DIR}" "-DENGINE=${CMAKE_CURRENT_SOURCE_DIR}"
            "-DCACHE=${CMAKE_BINARY_DIR}/CMakeCache.txt"
            "-DSETTINGS=${CMAKE_CURRENT_BINARY_DIR}/distribution-target-settings.txt"
            -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/WGStampBuildInfo.cmake"
    COMMAND ${CMAKE_COMMAND} -E make_directory
        "${WG_LIBRARY_RELEASE_DIR}/LICENSES"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "$<TARGET_FILE:wolf3d>"
        "${WG_LIBRARY_RELEASE_DIR}/$<TARGET_FILE_NAME:wolf3d>"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_SOURCE_DIR}/include/WOLF3D.h"
        "${WG_LIBRARY_RELEASE_DIR}/WOLF3D.h"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_SOURCE_DIR}/include/WOLF3D_STDINT.h"
        "${WG_LIBRARY_RELEASE_DIR}/WOLF3D_STDINT.h"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_SOURCE_DIR}/LICENSE"
        "${WG_LIBRARY_RELEASE_DIR}/LICENSE.txt"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_SOURCE_DIR}/THIRD_PARTY.md"
        "${WG_LIBRARY_RELEASE_DIR}/THIRD_PARTY_NOTICES.txt"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_BINARY_DIR}/LIBRARY-README.txt"
        "${WG_LIBRARY_RELEASE_DIR}/README.txt"
)
if(WG_ENABLE_OPL_NUKED)
    list(APPEND WG_LIBRARY_RELEASE_COMMANDS
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "$<TARGET_FILE:nuked-opl3>"
            "${WG_LIBRARY_RELEASE_DIR}/$<TARGET_FILE_NAME:nuked-opl3>"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_SOURCE_DIR}/third_party/Nuked-OPL3/LICENSE"
            "${WG_LIBRARY_RELEASE_DIR}/LICENSES/Nuked-OPL3-LGPL-2.1.txt"
    )
endif()
if(WG_ENABLE_OPL_DBOPL)
    list(APPEND WG_LIBRARY_RELEASE_COMMANDS
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_SOURCE_DIR}/third_party/DBOPL/README.wolf3d-lib.md"
            "${WG_LIBRARY_RELEASE_DIR}/README.DBOPL.txt"
    )
endif()
if(WIN32)
    list(APPEND WG_LIBRARY_RELEASE_COMMANDS
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "$<TARGET_LINKER_FILE:wolf3d>"
            "${WG_LIBRARY_RELEASE_DIR}/$<TARGET_LINKER_FILE_NAME:wolf3d>"
    )
elseif(UNIX AND NOT APPLE)
    list(APPEND WG_LIBRARY_RELEASE_COMMANDS
        COMMAND ${CMAKE_COMMAND} -E create_symlink
            "$<TARGET_FILE_NAME:wolf3d>"
            "${WG_LIBRARY_RELEASE_DIR}/$<TARGET_SONAME_FILE_NAME:wolf3d>"
        COMMAND ${CMAKE_COMMAND} -E create_symlink
            "$<TARGET_SONAME_FILE_NAME:wolf3d>"
            "${WG_LIBRARY_RELEASE_DIR}/$<TARGET_LINKER_FILE_NAME:wolf3d>"
    )
endif()
set(WG_LIBRARY_RELEASE_DEPENDS wolf3d)
if(WG_OPL_LIBRARY_TARGETS)
    list(APPEND WG_LIBRARY_RELEASE_DEPENDS ${WG_OPL_LIBRARY_TARGETS})
endif()
add_custom_target(library-release
    ${WG_LIBRARY_RELEASE_COMMANDS}
    DEPENDS ${WG_LIBRARY_RELEASE_DEPENDS}
    COMMENT "Staging ${WG_RELEASE_ARCH} wolf3d shared library"
    VERBATIM
)

function(wg_configure_host target engine_target)
    target_link_libraries(${target} PRIVATE ${engine_target})
    target_compile_features(${target} PRIVATE c_std_99)
    if(MINGW AND WG_STATIC_GNU_RUNTIME)
        target_link_options(${target} PRIVATE -static-libgcc)
    endif()
    if(WG_MSVC_COMPAT_INCLUDE)
        target_include_directories(${target} PRIVATE
            "${WG_MSVC_COMPAT_INCLUDE}")
    endif()

    if(MSVC)
        target_compile_options(${target} PRIVATE /W4)
        if(MSVC_VERSION LESS 1900)
            target_compile_definitions(${target} PRIVATE inline=__inline)
        endif()
        if(MSVC_VERSION GREATER_EQUAL 1910)
            target_compile_options(${target} PRIVATE /permissive-)
        endif()
        if(WG_WARNINGS_AS_ERRORS)
            target_compile_options(${target} PRIVATE /WX)
        endif()
    else()
        target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic)
        if(WG_WARNINGS_AS_ERRORS)
            target_compile_options(${target} PRIVATE -Werror)
        endif()
    endif()
endfunction()

if(WG_BUILD_HEADLESS)
    add_executable(wolf3dgeneric-headless
        platforms/WG_TEXT_OUTPUT.c
        platforms/headless/MAIN.c
        platforms/headless/WG_HEADLESS.c
    )
    wg_configure_host(wolf3dgeneric-headless wolf3d-internal)
    target_include_directories(wolf3dgeneric-headless PRIVATE src)
endif()

if(BUILD_TESTING)
    add_executable(wolf3d-public-api-consumer
        tests/WOLF3D_CONSUMER.c
    )
    wg_configure_host(wolf3d-public-api-consumer wolf3d)
    add_test(NAME public-api-consumer COMMAND wolf3d-public-api-consumer)
endif()

if(BUILD_TESTING AND WG_BUILD_HEADLESS)
    add_test(NAME headless-smoke COMMAND wolf3dgeneric-headless --headless-smoke)
    if(WG_ENABLE_OPL_NUKED)
        add_test(NAME runtime-opl-nuked COMMAND wolf3dgeneric-headless
            --headless-smoke --opl nuked --sample-rate 44100)
    endif()
    if(WG_ENABLE_OPL_DBOPL)
        add_test(NAME runtime-opl-dbopl COMMAND wolf3dgeneric-headless
            --headless-smoke --opl dbopl --sample-rate 48000)
    endif()
    if(WG_ENABLE_OPL_SILENT)
        add_test(NAME runtime-opl-silent COMMAND wolf3dgeneric-headless
            --headless-smoke --opl silent --sample-rate 44100)
    endif()

    add_executable(wg-tests
        platforms/WG_TEXT_OUTPUT.c
        tests/WG_TESTS.c
    )
    if(WG_DEFAULT_OPL_DRIVER STREQUAL "silent")
        target_compile_definitions(wg-tests PRIVATE WG_AUDIO_SILENT=1)
    elseif(WG_DEFAULT_OPL_DRIVER STREQUAL "dbopl")
        target_compile_definitions(wg-tests PRIVATE WG_OPL_DBOPL=1)
    endif()
    wg_configure_host(wg-tests wolf3d-internal)
    if(MSVC AND MSVC_VERSION LESS 1900)
        # The assertion macros intentionally use do/while (0). Older MSVC
        # diagnoses every expansion as a constant conditional at /W4.
        target_compile_options(wg-tests PRIVATE /wd4127)
    endif()
    target_include_directories(wg-tests PRIVATE platforms src)
    add_test(NAME unit-tests COMMAND wg-tests)

    set(WG_TEST_WL1_PATH "" CACHE PATH "Path to external Wolf3D v1.4 WL1 data")
    set(WG_TEST_WL6_PATH "" CACHE PATH "Path to external Wolf3D v1.4 WL6 data")
    set(WG_TEST_WL6_APOGEE_PATH "" CACHE PATH
        "Path to external Wolf3D v1.4 Apogee WL6 data")
    set(WG_TEST_WL6_GOG_PATH "" CACHE PATH
        "Path to external Wolf3D v1.4 Activision/GOG WL6 data")
    set(WG_TEST_SOD_PATH "" CACHE PATH "Path to external Spear of Destiny SOD data")
    set(WG_TEST_SDM_PATH "" CACHE PATH "Path to external Spear of Destiny demo SDM data")
    set(WG_TEST_SD1_PATH "" CACHE PATH "Path to external Spear mission 1 data")
    set(WG_TEST_SD2_PATH "" CACHE PATH "Path to external Spear mission 2 data")
    set(WG_TEST_SD3_PATH "" CACHE PATH "Path to external Spear mission 3 data")
    if(WG_TEST_WL1_PATH)
        add_test(NAME data-wl1 COMMAND wg-tests --data wl1 "${WG_TEST_WL1_PATH}")
        add_test(NAME signon-auto-wl1 COMMAND wolf3dgeneric-headless
            --data "${WG_TEST_WL1_PATH}" --game WL1 --signon auto
            --frame-hash --palette-hash)
        set_tests_properties(signon-auto-wl1 PROPERTIES
            PASS_REGULAR_EXPRESSION "c4439e1ef4a9b8f5.*1e14f48394b7e6fd")
        add_test(NAME rating-view-wl1 COMMAND wolf3dgeneric-headless
            --data "${WG_TEST_WL1_PATH}" --game WL1 --rating-view
            --frame-hash --palette-hash)
        set_tests_properties(rating-view-wl1 PROPERTIES
            PASS_REGULAR_EXPRESSION "f49d01316c7fdd4c.*1e14f48394b7e6fd")
        add_test(NAME boss-view-wl1 COMMAND wolf3dgeneric-headless
            --data "${WG_TEST_WL1_PATH}" --play-view --map 8 --boss-view
            --frame-hash)
        set_tests_properties(boss-view-wl1 PROPERTIES
            PASS_REGULAR_EXPRESSION "1d485eba8c33cbaf")
        add_test(NAME boss-fire-view-wl1 COMMAND wolf3dgeneric-headless
            --data "${WG_TEST_WL1_PATH}" --play-view --map 8
            --boss-fire-view --frame-hash)
        set_tests_properties(boss-fire-view-wl1 PROPERTIES
            PASS_REGULAR_EXPRESSION "e27e5ef07f402c3a")
        add_test(NAME patrol-view-wl1 COMMAND wolf3dgeneric-headless
            --data "${WG_TEST_WL1_PATH}" --play-view --open-doors
            --patrol-view --actor-tics 127 --frame-hash)
        set_tests_properties(patrol-view-wl1 PROPERTIES
            PASS_REGULAR_EXPRESSION "75d12ce4aeaec8d1")
        add_test(NAME alert-view-wl1 COMMAND wolf3dgeneric-headless
            --data "${WG_TEST_WL1_PATH}" --play-view --alert-view
            --frame-hash)
        set_tests_properties(alert-view-wl1 PROPERTIES
            PASS_REGULAR_EXPRESSION "10e6284c2cb7c793")
        add_test(NAME chase-view-wl1 COMMAND wolf3dgeneric-headless
            --data "${WG_TEST_WL1_PATH}" --play-view --chase-view
            --actor-tics 9 --frame-hash)
        set_tests_properties(chase-view-wl1 PROPERTIES
            PASS_REGULAR_EXPRESSION "7f01ac2289944007")
        add_test(NAME fire-view-wl1 COMMAND wolf3dgeneric-headless
            --data "${WG_TEST_WL1_PATH}" --play-view --fire-view
            --frame-hash)
        set_tests_properties(fire-view-wl1 PROPERTIES
            PASS_REGULAR_EXPRESSION "f7767bb72bfd1b79")
        add_test(NAME bite-view-wl1 COMMAND wolf3dgeneric-headless
            --data "${WG_TEST_WL1_PATH}" --play-view --bite-view
            --frame-hash)
        set_tests_properties(bite-view-wl1 PROPERTIES
            PASS_REGULAR_EXPRESSION "1e94823c347ded46")
        add_test(NAME pushwall-view-wl1 COMMAND wolf3dgeneric-headless
            --data "${WG_TEST_WL1_PATH}" --play-view --pushwall-view
            --frame-hash)
        set_tests_properties(pushwall-view-wl1 PROPERTIES
            PASS_REGULAR_EXPRESSION "1063bf9d8a8c023c")
        add_test(NAME death-view-wl1 COMMAND wolf3dgeneric-headless
            --data "${WG_TEST_WL1_PATH}" --play-view --death-view
            --frame-hash)
        set_tests_properties(death-view-wl1 PROPERTIES
            PASS_REGULAR_EXPRESSION "d1b4d08b65136c4a")
        add_test(NAME death-start-view-wl1 COMMAND wolf3dgeneric-headless
            --data "${WG_TEST_WL1_PATH}" --play-view --death-view
            --actor-tics 1 --frame-hash)
        set_tests_properties(death-start-view-wl1 PROPERTIES
            PASS_REGULAR_EXPRESSION "5fb4e479dfc0f822")
        add_test(NAME death-terminal-view-wl1 COMMAND wolf3dgeneric-headless
            --data "${WG_TEST_WL1_PATH}" --play-view --death-view
            --actor-tics 45 --frame-hash)
        set_tests_properties(death-terminal-view-wl1 PROPERTIES
            PASS_REGULAR_EXPRESSION "1ba7b95287310b8a")
        add_test(NAME boss-death-view-wl1 COMMAND wolf3dgeneric-headless
            --data "${WG_TEST_WL1_PATH}" --play-view --map 8
            --boss-death-view --frame-hash)
        set_tests_properties(boss-death-view-wl1 PROPERTIES
            PASS_REGULAR_EXPRESSION "2146e49de8e66017")
        add_test(NAME player-fire-view-wl1 COMMAND wolf3dgeneric-headless
            --data "${WG_TEST_WL1_PATH}" --play-view --player-fire-view
            --frame-hash)
        set_tests_properties(player-fire-view-wl1 PROPERTIES
            PASS_REGULAR_EXPRESSION "987963e7d9ae3f41")
        add_test(NAME pickup-view-wl1 COMMAND wolf3dgeneric-headless
            --data "${WG_TEST_WL1_PATH}" --play-view --pickup-view
            --frame-hash)
        set_tests_properties(pickup-view-wl1 PROPERTIES
            PASS_REGULAR_EXPRESSION "b2802bfd325cdb5f")
        add_test(NAME door-use-view-wl1 COMMAND wolf3dgeneric-headless
            --data "${WG_TEST_WL1_PATH}" --play-view --door-use-view
            --frame-hash)
        set_tests_properties(door-use-view-wl1 PROPERTIES
            PASS_REGULAR_EXPRESSION "5153e09ccb96ebd1")
        add_test(NAME play-loop-forward-wl1 COMMAND wolf3dgeneric-headless
            --data "${WG_TEST_WL1_PATH}" --play-view --forward-tics 35
            --frame-hash)
        set_tests_properties(play-loop-forward-wl1 PROPERTIES
            PASS_REGULAR_EXPRESSION "1f05f4ccdb8f7c71")
        add_test(NAME next-floor-view-wl1 COMMAND wolf3dgeneric-headless
            --data "${WG_TEST_WL1_PATH}" --play-view --map 1
            --frame-hash)
        set_tests_properties(next-floor-view-wl1 PROPERTIES
            PASS_REGULAR_EXPRESSION "5b83eae539383e97")
        add_test(NAME pause-view-wl1 COMMAND wolf3dgeneric-headless
            --data "${WG_TEST_WL1_PATH}" --play-view --pause-view
            --frame-hash)
        set_tests_properties(pause-view-wl1 PROPERTIES
            PASS_REGULAR_EXPRESSION "bb1ee429bb96d3a4")
        add_test(NAME intermission-view-wl1 COMMAND wolf3dgeneric-headless
            --data "${WG_TEST_WL1_PATH}" --play-view --intermission-view
            --frame-hash)
        set_tests_properties(intermission-view-wl1 PROPERTIES
            PASS_REGULAR_EXPRESSION "85b3dfdb33f1fb4d")
        add_test(NAME damage-flash-view-wl1 COMMAND wolf3dgeneric-headless
            --data "${WG_TEST_WL1_PATH}" --play-view --damage-flash-view
            --palette-hash)
        set_tests_properties(damage-flash-view-wl1 PROPERTIES
            PASS_REGULAR_EXPRESSION "d6969024db3bdb40")
        add_test(NAME bonus-flash-view-wl1 COMMAND wolf3dgeneric-headless
            --data "${WG_TEST_WL1_PATH}" --play-view --bonus-flash-view
            --palette-hash)
        set_tests_properties(bonus-flash-view-wl1 PROPERTIES
            PASS_REGULAR_EXPRESSION "8b16358ec3225130")
        add_test(NAME player-death-view-wl1 COMMAND wolf3dgeneric-headless
            --data "${WG_TEST_WL1_PATH}" --play-view --player-death-view
            --frame-hash)
        set_tests_properties(player-death-view-wl1 PROPERTIES
            PASS_REGULAR_EXPRESSION "b0dc74173cae5930")
        add_test(NAME victory-view-wl1 COMMAND wolf3dgeneric-headless
            --data "${WG_TEST_WL1_PATH}" --play-view --victory-view
            --frame-hash)
        set_tests_properties(victory-view-wl1 PROPERTIES
            PASS_REGULAR_EXPRESSION "8e828f1563f3c064")
        add_test(NAME end-text-view-wl1 COMMAND wolf3dgeneric-headless
            --data "${WG_TEST_WL1_PATH}" --play-view --end-text-view
            --frame-hash)
        set_tests_properties(end-text-view-wl1 PROPERTIES
            PASS_REGULAR_EXPRESSION "9751906604fce502")
        add_test(NAME main-menu-view-wl1 COMMAND wolf3dgeneric-headless
            --data "${WG_TEST_WL1_PATH}" --play-view --main-menu-view
            --frame-hash)
        set_tests_properties(main-menu-view-wl1 PROPERTIES
            PASS_REGULAR_EXPRESSION "cdbff8b31548b64e")
        add_test(NAME in-game-menu-view-wl1 COMMAND wolf3dgeneric-headless
            --data "${WG_TEST_WL1_PATH}" --play-view --in-game-menu-view
            --frame-hash)
        set_tests_properties(in-game-menu-view-wl1 PROPERTIES
            PASS_REGULAR_EXPRESSION "9d4fae41d9d29488")
        add_test(NAME load-game-view-wl1 COMMAND wolf3dgeneric-headless
            --data "${WG_TEST_WL1_PATH}" --play-view --load-game-view
            --frame-hash)
        set_tests_properties(load-game-view-wl1 PROPERTIES
            PASS_REGULAR_EXPRESSION "d81cecaa1eef0887")
        add_test(NAME save-game-view-wl1 COMMAND wolf3dgeneric-headless
            --data "${WG_TEST_WL1_PATH}" --play-view --save-game-view
            --frame-hash)
        set_tests_properties(save-game-view-wl1 PROPERTIES
            PASS_REGULAR_EXPRESSION "55f762913ddab4c7")
        add_test(NAME sound-menu-view-wl1 COMMAND wolf3dgeneric-headless
            --data "${WG_TEST_WL1_PATH}" --play-view --sound-menu-view
            --frame-hash)
        set_tests_properties(sound-menu-view-wl1 PROPERTIES
            PASS_REGULAR_EXPRESSION "bd51c51cceb48736")
        add_test(NAME control-menu-view-wl1 COMMAND wolf3dgeneric-headless
            --data "${WG_TEST_WL1_PATH}" --play-view --control-menu-view
            --frame-hash)
        set_tests_properties(control-menu-view-wl1 PROPERTIES
            PASS_REGULAR_EXPRESSION "59ee44f4e7ffa03d")
        add_test(NAME joystick-menu-view-wl1 COMMAND wolf3dgeneric-headless
            --data "${WG_TEST_WL1_PATH}" --play-view --joystick-menu-view
            --frame-hash)
        set_tests_properties(joystick-menu-view-wl1 PROPERTIES
            PASS_REGULAR_EXPRESSION "1800366abc1456d1")
        add_test(NAME mouse-sensitivity-view-wl1
            COMMAND wolf3dgeneric-headless
            --data "${WG_TEST_WL1_PATH}" --play-view
            --mouse-sensitivity-view --frame-hash)
        set_tests_properties(mouse-sensitivity-view-wl1 PROPERTIES
            PASS_REGULAR_EXPRESSION "f7dc4f44ff382c6b")
        add_test(NAME customize-controls-view-wl1
            COMMAND wolf3dgeneric-headless
            --data "${WG_TEST_WL1_PATH}" --play-view
            --customize-controls-view --frame-hash)
        set_tests_properties(customize-controls-view-wl1 PROPERTIES
            PASS_REGULAR_EXPRESSION "b6579550b7d1e9be")
        add_test(NAME change-view-wl1
            COMMAND wolf3dgeneric-headless
            --data "${WG_TEST_WL1_PATH}" --play-view
            --change-view --frame-hash)
        set_tests_properties(change-view-wl1 PROPERTIES
            PASS_REGULAR_EXPRESSION "fe964b945ec6d203")
        add_test(NAME default-view-size-wl1
            COMMAND wolf3dgeneric-headless
            --data "${WG_TEST_WL1_PATH}" --play-view
            --view-size 15 --frame-hash)
        set_tests_properties(default-view-size-wl1 PROPERTIES
            PASS_REGULAR_EXPRESSION "65cf0177bb1dcab7")
        add_test(NAME credits-view-wl1 COMMAND wolf3dgeneric-headless
            --data "${WG_TEST_WL1_PATH}" --play-view --credits-view
            --frame-hash)
        set_tests_properties(credits-view-wl1 PROPERTIES
            PASS_REGULAR_EXPRESSION "877dd0b7c8d5bf6d")
        add_test(NAME help-view-wl1 COMMAND wolf3dgeneric-headless
            --data "${WG_TEST_WL1_PATH}" --play-view --help-view
            --frame-hash)
        set_tests_properties(help-view-wl1 PROPERTIES
            PASS_REGULAR_EXPRESSION "afa188f7e955ec42")
        add_test(NAME demo-view-wl1 COMMAND wolf3dgeneric-headless
            --data "${WG_TEST_WL1_PATH}" --play-view --demo-view
            --demo-commands 70 --frame-hash)
        set_tests_properties(demo-view-wl1 PROPERTIES
            PASS_REGULAR_EXPRESSION "a54168bd5f03ee1a")
        add_test(NAME demo-complete-0-wl1 COMMAND wolf3dgeneric-headless
            --data "${WG_TEST_WL1_PATH}" --play-view --demo-view
            --demo-number 0 --demo-commands 1152 --frame-hash)
        set_tests_properties(demo-complete-0-wl1 PROPERTIES
            PASS_REGULAR_EXPRESSION "9192ed26f4b29340")
        add_test(NAME demo-complete-1-wl1 COMMAND wolf3dgeneric-headless
            --data "${WG_TEST_WL1_PATH}" --play-view --demo-view
            --demo-number 1 --demo-commands 1284 --frame-hash)
        set_tests_properties(demo-complete-1-wl1 PROPERTIES
            PASS_REGULAR_EXPRESSION "38a09a4befc1607b")
        add_test(NAME demo-complete-2-wl1 COMMAND wolf3dgeneric-headless
            --data "${WG_TEST_WL1_PATH}" --play-view --demo-view
            --demo-number 2 --demo-commands 671 --frame-hash)
        set_tests_properties(demo-complete-2-wl1 PROPERTIES
            PASS_REGULAR_EXPRESSION "c8979588ac87cbd3")
        add_test(NAME demo-complete-3-wl1 COMMAND wolf3dgeneric-headless
            --data "${WG_TEST_WL1_PATH}" --play-view --demo-view
            --demo-number 3 --demo-commands 633 --frame-hash)
        set_tests_properties(demo-complete-3-wl1 PROPERTIES
            PASS_REGULAR_EXPRESSION "7dbe2cf7c8efc5f4")
        add_test(NAME episode-menu-view-wl1 COMMAND wolf3dgeneric-headless
            --data "${WG_TEST_WL1_PATH}" --play-view --episode-menu-view
            --frame-hash)
        set_tests_properties(episode-menu-view-wl1 PROPERTIES
            PASS_REGULAR_EXPRESSION "23ad3114db625b9b")
        add_test(NAME difficulty-menu-view-wl1 COMMAND wolf3dgeneric-headless
            --data "${WG_TEST_WL1_PATH}" --play-view --difficulty-menu-view
            --frame-hash)
        set_tests_properties(difficulty-menu-view-wl1 PROPERTIES
            PASS_REGULAR_EXPRESSION "6ef5da5dc52ae5b0")
        add_test(NAME high-score-view-wl1 COMMAND wolf3dgeneric-headless
            --data "${WG_TEST_WL1_PATH}" --play-view --high-score-view
            --frame-hash)
        set_tests_properties(high-score-view-wl1 PROPERTIES
            PASS_REGULAR_EXPRESSION "7b063c0fb260132e")
        add_test(NAME high-score-entry-view-wl1 COMMAND wolf3dgeneric-headless
            --data "${WG_TEST_WL1_PATH}" --play-view
            --high-score-entry-view --frame-hash)
        set_tests_properties(high-score-entry-view-wl1 PROPERTIES
            PASS_REGULAR_EXPRESSION "3b75e429fe454ad8")
    endif()
    if(WG_TEST_WL6_APOGEE_PATH)
        add_test(NAME data-wl6-apogee COMMAND wg-tests --data wl6-apogee
            "${WG_TEST_WL6_APOGEE_PATH}")
        add_test(NAME signon-auto-wl6-apogee COMMAND wolf3dgeneric-headless
            --data "${WG_TEST_WL6_APOGEE_PATH}" --game WL6 --signon auto
            --frame-hash --palette-hash)
        set_tests_properties(signon-auto-wl6-apogee PROPERTIES
            PASS_REGULAR_EXPRESSION "c4439e1ef4a9b8f5.*1e14f48394b7e6fd")
        add_test(NAME main-menu-view-wl6-apogee
            COMMAND wolf3dgeneric-headless
            --data "${WG_TEST_WL6_APOGEE_PATH}" --game WL6 --play-view
            --main-menu-view --frame-hash)
        set_tests_properties(main-menu-view-wl6-apogee PROPERTIES
            PASS_REGULAR_EXPRESSION "cdbff8b31548b64e")
        add_test(NAME help-view-wl6-apogee COMMAND wolf3dgeneric-headless
            --data "${WG_TEST_WL6_APOGEE_PATH}" --game WL6 --play-view
            --help-view --frame-hash --palette-hash)
        set_tests_properties(help-view-wl6-apogee PROPERTIES
            PASS_REGULAR_EXPRESSION "afa188f7e955ec42.*1e14f48394b7e6fd")
        add_test(NAME end-text-view-wl6-apogee
            COMMAND wolf3dgeneric-headless
            --data "${WG_TEST_WL6_APOGEE_PATH}" --game WL6 --play-view
            --end-text-view --frame-hash)
        set_tests_properties(end-text-view-wl6-apogee PROPERTIES
            PASS_REGULAR_EXPRESSION "9751906604fce502")
        add_test(NAME demo-complete-0-wl6-apogee
            COMMAND wolf3dgeneric-headless
            --data "${WG_TEST_WL6_APOGEE_PATH}" --game WL6 --play-view
            --demo-view --demo-number 0 --demo-commands 691 --frame-hash)
        set_tests_properties(demo-complete-0-wl6-apogee PROPERTIES
            PASS_REGULAR_EXPRESSION "a83159039d84ca9c")
    endif()
    if(WG_TEST_WL6_PATH)
        add_test(NAME data-wl6 COMMAND wg-tests --data wl6 "${WG_TEST_WL6_PATH}")
        add_test(NAME signon-auto-wl6 COMMAND wolf3dgeneric-headless
            --data "${WG_TEST_WL6_PATH}" --game WL6 --signon auto
            --frame-hash --palette-hash)
        set_tests_properties(signon-auto-wl6 PROPERTIES
            PASS_REGULAR_EXPRESSION "7f78b83ef22e6fbc.*1e14f48394b7e6fd")
        add_test(NAME signon-mouse-wl6 COMMAND wolf3dgeneric-headless
            --data "${WG_TEST_WL6_PATH}" --game WL6 --signon auto --mouse
            --frame-hash --palette-hash)
        set_tests_properties(signon-mouse-wl6 PROPERTIES
            PASS_REGULAR_EXPRESSION "f83e3e048c4550f4.*1e14f48394b7e6fd")
        add_test(NAME signon-adlib-wl6 COMMAND wolf3dgeneric-headless
            --data "${WG_TEST_WL6_PATH}" --game WL6 --signon auto --adlib
            --frame-hash --palette-hash)
        set_tests_properties(signon-adlib-wl6 PROPERTIES
            PASS_REGULAR_EXPRESSION "d20c9b30e730b7ac.*1e14f48394b7e6fd")
        add_test(NAME rating-view-wl6 COMMAND wolf3dgeneric-headless
            --data "${WG_TEST_WL6_PATH}" --game WL6 --rating-view
            --frame-hash --palette-hash)
        set_tests_properties(rating-view-wl6 PROPERTIES
            PASS_REGULAR_EXPRESSION "f49d01316c7fdd4c.*1e14f48394b7e6fd")
        add_test(NAME boss-view-wl6 COMMAND wolf3dgeneric-headless
            --data "${WG_TEST_WL6_PATH}" --play-view --map 8 --boss-view
            --frame-hash)
        set_tests_properties(boss-view-wl6 PROPERTIES
            PASS_REGULAR_EXPRESSION "1d485eba8c33cbaf")
        add_test(NAME boss-fire-view-wl6 COMMAND wolf3dgeneric-headless
            --data "${WG_TEST_WL6_PATH}" --play-view --map 8
            --boss-fire-view --frame-hash)
        set_tests_properties(boss-fire-view-wl6 PROPERTIES
            PASS_REGULAR_EXPRESSION "e27e5ef07f402c3a")
        add_test(NAME patrol-view-wl6 COMMAND wolf3dgeneric-headless
            --data "${WG_TEST_WL6_PATH}" --play-view --open-doors
            --patrol-view --actor-tics 127 --frame-hash)
        set_tests_properties(patrol-view-wl6 PROPERTIES
            PASS_REGULAR_EXPRESSION "75d12ce4aeaec8d1")
        add_test(NAME alert-view-wl6 COMMAND wolf3dgeneric-headless
            --data "${WG_TEST_WL6_PATH}" --play-view --alert-view
            --frame-hash)
        set_tests_properties(alert-view-wl6 PROPERTIES
            PASS_REGULAR_EXPRESSION "10e6284c2cb7c793")
        add_test(NAME chase-view-wl6 COMMAND wolf3dgeneric-headless
            --data "${WG_TEST_WL6_PATH}" --play-view --chase-view
            --actor-tics 9 --frame-hash)
        set_tests_properties(chase-view-wl6 PROPERTIES
            PASS_REGULAR_EXPRESSION "7f01ac2289944007")
        add_test(NAME fire-view-wl6 COMMAND wolf3dgeneric-headless
            --data "${WG_TEST_WL6_PATH}" --play-view --fire-view
            --frame-hash)
        set_tests_properties(fire-view-wl6 PROPERTIES
            PASS_REGULAR_EXPRESSION "f7767bb72bfd1b79")
        add_test(NAME bite-view-wl6 COMMAND wolf3dgeneric-headless
            --data "${WG_TEST_WL6_PATH}" --play-view --bite-view
            --frame-hash)
        set_tests_properties(bite-view-wl6 PROPERTIES
            PASS_REGULAR_EXPRESSION "1e94823c347ded46")
        add_test(NAME needle-view-wl6 COMMAND wolf3dgeneric-headless
            --data "${WG_TEST_WL6_PATH}" --play-view --map 18
            --needle-view --frame-hash)
        set_tests_properties(needle-view-wl6 PROPERTIES
            PASS_REGULAR_EXPRESSION "3236987d899f0630")
        add_test(NAME rocket-view-wl6 COMMAND wolf3dgeneric-headless
            --data "${WG_TEST_WL6_PATH}" --play-view --map 38
            --rocket-view --frame-hash)
        set_tests_properties(rocket-view-wl6 PROPERTIES
            PASS_REGULAR_EXPRESSION "5bf01b790397a154")
        add_test(NAME flame-view-wl6 COMMAND wolf3dgeneric-headless
            --data "${WG_TEST_WL6_PATH}" --play-view --map 28
            --flame-view --frame-hash)
        set_tests_properties(flame-view-wl6 PROPERTIES
            PASS_REGULAR_EXPRESSION "093b1a496a6e809b")
        add_test(NAME pushwall-view-wl6 COMMAND wolf3dgeneric-headless
            --data "${WG_TEST_WL6_PATH}" --play-view --pushwall-view
            --frame-hash)
        set_tests_properties(pushwall-view-wl6 PROPERTIES
            PASS_REGULAR_EXPRESSION "1063bf9d8a8c023c")
        add_test(NAME death-view-wl6 COMMAND wolf3dgeneric-headless
            --data "${WG_TEST_WL6_PATH}" --play-view --death-view
            --frame-hash)
        set_tests_properties(death-view-wl6 PROPERTIES
            PASS_REGULAR_EXPRESSION "d1b4d08b65136c4a")
        add_test(NAME boss-death-view-wl6 COMMAND wolf3dgeneric-headless
            --data "${WG_TEST_WL6_PATH}" --play-view --map 8
            --boss-death-view --frame-hash)
        set_tests_properties(boss-death-view-wl6 PROPERTIES
            PASS_REGULAR_EXPRESSION "2146e49de8e66017")
        add_test(NAME player-fire-view-wl6 COMMAND wolf3dgeneric-headless
            --data "${WG_TEST_WL6_PATH}" --play-view --player-fire-view
            --frame-hash)
        set_tests_properties(player-fire-view-wl6 PROPERTIES
            PASS_REGULAR_EXPRESSION "987963e7d9ae3f41")
        add_test(NAME pickup-view-wl6 COMMAND wolf3dgeneric-headless
            --data "${WG_TEST_WL6_PATH}" --play-view --pickup-view
            --frame-hash)
        set_tests_properties(pickup-view-wl6 PROPERTIES
            PASS_REGULAR_EXPRESSION "b2802bfd325cdb5f")
        add_test(NAME door-use-view-wl6 COMMAND wolf3dgeneric-headless
            --data "${WG_TEST_WL6_PATH}" --play-view --door-use-view
            --frame-hash)
        set_tests_properties(door-use-view-wl6 PROPERTIES
            PASS_REGULAR_EXPRESSION "5153e09ccb96ebd1")
        add_test(NAME play-loop-forward-wl6 COMMAND wolf3dgeneric-headless
            --data "${WG_TEST_WL6_PATH}" --play-view --forward-tics 35
            --frame-hash)
        set_tests_properties(play-loop-forward-wl6 PROPERTIES
            PASS_REGULAR_EXPRESSION "1f05f4ccdb8f7c71")
        add_test(NAME next-floor-view-wl6 COMMAND wolf3dgeneric-headless
            --data "${WG_TEST_WL6_PATH}" --play-view --map 1
            --frame-hash)
        set_tests_properties(next-floor-view-wl6 PROPERTIES
            PASS_REGULAR_EXPRESSION "5b83eae539383e97")
        add_test(NAME pause-view-wl6 COMMAND wolf3dgeneric-headless
            --data "${WG_TEST_WL6_PATH}" --play-view --pause-view
            --frame-hash)
        set_tests_properties(pause-view-wl6 PROPERTIES
            PASS_REGULAR_EXPRESSION "bb1ee429bb96d3a4")
        add_test(NAME intermission-view-wl6 COMMAND wolf3dgeneric-headless
            --data "${WG_TEST_WL6_PATH}" --play-view --intermission-view
            --frame-hash)
        set_tests_properties(intermission-view-wl6 PROPERTIES
            PASS_REGULAR_EXPRESSION "85b3dfdb33f1fb4d")
        add_test(NAME damage-flash-view-wl6 COMMAND wolf3dgeneric-headless
            --data "${WG_TEST_WL6_PATH}" --play-view --damage-flash-view
            --palette-hash)
        set_tests_properties(damage-flash-view-wl6 PROPERTIES
            PASS_REGULAR_EXPRESSION "d6969024db3bdb40")
        add_test(NAME bonus-flash-view-wl6 COMMAND wolf3dgeneric-headless
            --data "${WG_TEST_WL6_PATH}" --play-view --bonus-flash-view
            --palette-hash)
        set_tests_properties(bonus-flash-view-wl6 PROPERTIES
            PASS_REGULAR_EXPRESSION "8b16358ec3225130")
        add_test(NAME player-death-view-wl6 COMMAND wolf3dgeneric-headless
            --data "${WG_TEST_WL6_PATH}" --play-view --player-death-view
            --frame-hash)
        set_tests_properties(player-death-view-wl6 PROPERTIES
            PASS_REGULAR_EXPRESSION "b0dc74173cae5930")
        add_test(NAME victory-view-wl6 COMMAND wolf3dgeneric-headless
            --data "${WG_TEST_WL6_PATH}" --play-view --victory-view
            --frame-hash)
        set_tests_properties(victory-view-wl6 PROPERTIES
            PASS_REGULAR_EXPRESSION "8e828f1563f3c064")
        add_test(NAME end-text-view-wl6 COMMAND wolf3dgeneric-headless
            --data "${WG_TEST_WL6_PATH}" --play-view --end-text-view
            --frame-hash)
        set_tests_properties(end-text-view-wl6 PROPERTIES
            PASS_REGULAR_EXPRESSION "9751906604fce502")
        add_test(NAME main-menu-view-wl6 COMMAND wolf3dgeneric-headless
            --data "${WG_TEST_WL6_PATH}" --play-view --main-menu-view
            --frame-hash)
        set_tests_properties(main-menu-view-wl6 PROPERTIES
            PASS_REGULAR_EXPRESSION "f10ed9855ad7a846")
        add_test(NAME in-game-menu-view-wl6 COMMAND wolf3dgeneric-headless
            --data "${WG_TEST_WL6_PATH}" --play-view --in-game-menu-view
            --frame-hash)
        set_tests_properties(in-game-menu-view-wl6 PROPERTIES
            PASS_REGULAR_EXPRESSION "1315674b21b3bb10")
        add_test(NAME load-game-view-wl6 COMMAND wolf3dgeneric-headless
            --data "${WG_TEST_WL6_PATH}" --play-view --load-game-view
            --frame-hash)
        set_tests_properties(load-game-view-wl6 PROPERTIES
            PASS_REGULAR_EXPRESSION "d81cecaa1eef0887")
        add_test(NAME save-game-view-wl6 COMMAND wolf3dgeneric-headless
            --data "${WG_TEST_WL6_PATH}" --play-view --save-game-view
            --frame-hash)
        set_tests_properties(save-game-view-wl6 PROPERTIES
            PASS_REGULAR_EXPRESSION "55f762913ddab4c7")
        add_test(NAME sound-menu-view-wl6 COMMAND wolf3dgeneric-headless
            --data "${WG_TEST_WL6_PATH}" --play-view --sound-menu-view
            --frame-hash)
        set_tests_properties(sound-menu-view-wl6 PROPERTIES
            PASS_REGULAR_EXPRESSION "bd51c51cceb48736")
        add_test(NAME control-menu-view-wl6 COMMAND wolf3dgeneric-headless
            --data "${WG_TEST_WL6_PATH}" --play-view --control-menu-view
            --frame-hash)
        set_tests_properties(control-menu-view-wl6 PROPERTIES
            PASS_REGULAR_EXPRESSION "59ee44f4e7ffa03d")
        add_test(NAME joystick-menu-view-wl6 COMMAND wolf3dgeneric-headless
            --data "${WG_TEST_WL6_PATH}" --play-view --joystick-menu-view
            --frame-hash)
        set_tests_properties(joystick-menu-view-wl6 PROPERTIES
            PASS_REGULAR_EXPRESSION "1800366abc1456d1")
        add_test(NAME mouse-sensitivity-view-wl6
            COMMAND wolf3dgeneric-headless
            --data "${WG_TEST_WL6_PATH}" --play-view
            --mouse-sensitivity-view --frame-hash)
        set_tests_properties(mouse-sensitivity-view-wl6 PROPERTIES
            PASS_REGULAR_EXPRESSION "f7dc4f44ff382c6b")
        add_test(NAME customize-controls-view-wl6
            COMMAND wolf3dgeneric-headless
            --data "${WG_TEST_WL6_PATH}" --play-view
            --customize-controls-view --frame-hash)
        set_tests_properties(customize-controls-view-wl6 PROPERTIES
            PASS_REGULAR_EXPRESSION "b6579550b7d1e9be")
        add_test(NAME change-view-wl6
            COMMAND wolf3dgeneric-headless
            --data "${WG_TEST_WL6_PATH}" --play-view
            --change-view --frame-hash)
        set_tests_properties(change-view-wl6 PROPERTIES
            PASS_REGULAR_EXPRESSION "fe964b945ec6d203")
        add_test(NAME default-view-size-wl6
            COMMAND wolf3dgeneric-headless
            --data "${WG_TEST_WL6_PATH}" --play-view
            --view-size 15 --frame-hash)
        set_tests_properties(default-view-size-wl6 PROPERTIES
            PASS_REGULAR_EXPRESSION "65cf0177bb1dcab7")
        add_test(NAME credits-view-wl6 COMMAND wolf3dgeneric-headless
            --data "${WG_TEST_WL6_PATH}" --play-view --credits-view
            --frame-hash)
        set_tests_properties(credits-view-wl6 PROPERTIES
            PASS_REGULAR_EXPRESSION "877dd0b7c8d5bf6d")
        add_test(NAME help-view-wl6 COMMAND wolf3dgeneric-headless
            --data "${WG_TEST_WL6_PATH}" --play-view --help-view
            --frame-hash)
        set_tests_properties(help-view-wl6 PROPERTIES
            PASS_REGULAR_EXPRESSION "afa188f7e955ec42")
        add_test(NAME demo-view-wl6 COMMAND wolf3dgeneric-headless
            --data "${WG_TEST_WL6_PATH}" --play-view --demo-view
            --demo-commands 70 --frame-hash)
        set_tests_properties(demo-view-wl6 PROPERTIES
            PASS_REGULAR_EXPRESSION "c2c076ab6c8a6ac5")
        add_test(NAME demo-complete-0-wl6 COMMAND wolf3dgeneric-headless
            --data "${WG_TEST_WL6_PATH}" --play-view --demo-view
            --demo-number 0 --demo-commands 691 --frame-hash)
        set_tests_properties(demo-complete-0-wl6 PROPERTIES
            PASS_REGULAR_EXPRESSION "a83159039d84ca9c")
        add_test(NAME demo-complete-1-wl6 COMMAND wolf3dgeneric-headless
            --data "${WG_TEST_WL6_PATH}" --play-view --demo-view
            --demo-number 1 --demo-commands 1899 --frame-hash)
        set_tests_properties(demo-complete-1-wl6 PROPERTIES
            PASS_REGULAR_EXPRESSION "27d709a5dd215cac")
        add_test(NAME demo-complete-2-wl6 COMMAND wolf3dgeneric-headless
            --data "${WG_TEST_WL6_PATH}" --play-view --demo-view
            --demo-number 2 --demo-commands 1140 --frame-hash)
        set_tests_properties(demo-complete-2-wl6 PROPERTIES
            PASS_REGULAR_EXPRESSION "b60dad965cc64a47")
        add_test(NAME demo-complete-3-wl6 COMMAND wolf3dgeneric-headless
            --data "${WG_TEST_WL6_PATH}" --play-view --demo-view
            --demo-number 3 --demo-commands 1656 --frame-hash)
        set_tests_properties(demo-complete-3-wl6 PROPERTIES
            PASS_REGULAR_EXPRESSION "2d7ca110aa9658ba")
        add_test(NAME episode-menu-view-wl6 COMMAND wolf3dgeneric-headless
            --data "${WG_TEST_WL6_PATH}" --play-view --episode-menu-view
            --frame-hash)
        set_tests_properties(episode-menu-view-wl6 PROPERTIES
            PASS_REGULAR_EXPRESSION "45053c7411aec10f")
        add_test(NAME difficulty-menu-view-wl6 COMMAND wolf3dgeneric-headless
            --data "${WG_TEST_WL6_PATH}" --play-view --difficulty-menu-view
            --frame-hash)
        set_tests_properties(difficulty-menu-view-wl6 PROPERTIES
            PASS_REGULAR_EXPRESSION "6ef5da5dc52ae5b0")
        add_test(NAME high-score-view-wl6 COMMAND wolf3dgeneric-headless
            --data "${WG_TEST_WL6_PATH}" --play-view --high-score-view
            --frame-hash)
        set_tests_properties(high-score-view-wl6 PROPERTIES
            PASS_REGULAR_EXPRESSION "7b063c0fb260132e")
        add_test(NAME high-score-entry-view-wl6 COMMAND wolf3dgeneric-headless
            --data "${WG_TEST_WL6_PATH}" --play-view
            --high-score-entry-view --frame-hash)
        set_tests_properties(high-score-entry-view-wl6 PROPERTIES
            PASS_REGULAR_EXPRESSION "3b75e429fe454ad8")
    endif()
    if(WG_TEST_WL6_GOG_PATH)
        add_test(NAME data-wl6-gog COMMAND wg-tests --data wl6-gog
            "${WG_TEST_WL6_GOG_PATH}")
        add_test(NAME signon-auto-wl6-gog COMMAND wolf3dgeneric-headless
            --data "${WG_TEST_WL6_GOG_PATH}" --game WL6 --signon auto
            --frame-hash --palette-hash)
        set_tests_properties(signon-auto-wl6-gog PROPERTIES
            PASS_REGULAR_EXPRESSION "7cb4dca1b0143110.*1e14f48394b7e6fd")
    endif()
    if(WG_TEST_SOD_PATH)
        add_test(NAME signon-auto-sod COMMAND wolf3dgeneric-headless
            --data "${WG_TEST_SOD_PATH}" --game SOD --signon auto
            --frame-hash --palette-hash)
        set_tests_properties(signon-auto-sod PROPERTIES
            PASS_REGULAR_EXPRESSION "7e8d5e435a9f23a8.*eba126e8df00d1cb")
        add_test(NAME rating-view-sod COMMAND wolf3dgeneric-headless
            --data "${WG_TEST_SOD_PATH}" --game SOD --rating-view
            --frame-hash --palette-hash)
        set_tests_properties(rating-view-sod PROPERTIES
            PASS_REGULAR_EXPRESSION "f49d01316c7fdd4c.*eba126e8df00d1cb")
        add_test(NAME data-sod COMMAND wg-tests --spear-data SOD 666 21
            "${WG_TEST_SOD_PATH}")
        add_test(NAME title-view-sod COMMAND wolf3dgeneric-headless
            --data "${WG_TEST_SOD_PATH}" --game SOD --frame-hash
            --palette-hash)
        set_tests_properties(title-view-sod PROPERTIES
            PASS_REGULAR_EXPRESSION "7aa653c3de8139c6.*e2a15174c649db6a")
        add_test(NAME main-menu-view-sod COMMAND wolf3dgeneric-headless
            --data "${WG_TEST_SOD_PATH}" --game SOD --play-view
            --main-menu-view --frame-hash --palette-hash)
        set_tests_properties(main-menu-view-sod PROPERTIES
            PASS_REGULAR_EXPRESSION "74383419d65cb635.*eba126e8df00d1cb")
        add_test(NAME difficulty-menu-view-sod COMMAND wolf3dgeneric-headless
            --data "${WG_TEST_SOD_PATH}" --game SOD --play-view
            --difficulty-menu-view --frame-hash --palette-hash)
        set_tests_properties(difficulty-menu-view-sod PROPERTIES
            PASS_REGULAR_EXPRESSION "4b9dd46549814400.*eba126e8df00d1cb")
        add_test(NAME trans-view-sod COMMAND wolf3dgeneric-headless
            --data "${WG_TEST_SOD_PATH}" --game SOD --play-view --map 4
            --boss-view --frame-hash --palette-hash)
        set_tests_properties(trans-view-sod PROPERTIES
            PASS_REGULAR_EXPRESSION "2184a70f72440498.*eba126e8df00d1cb")
        add_test(NAME intermission-view-sod COMMAND wolf3dgeneric-headless
            --data "${WG_TEST_SOD_PATH}" --game SOD --play-view --map 4
            --intermission-view --frame-hash --palette-hash)
        set_tests_properties(intermission-view-sod PROPERTIES
            PASS_REGULAR_EXPRESSION "b29aa7321c8c4602.*eba126e8df00d1cb")
        add_test(NAME victory-collapse-view-sod COMMAND wolf3dgeneric-headless
            --data "${WG_TEST_SOD_PATH}" --game SOD --play-view
            --victory-collapse-view --collapse-frame 3
            --frame-hash --palette-hash)
        set_tests_properties(victory-collapse-view-sod PROPERTIES
            PASS_REGULAR_EXPRESSION "63c83359329a021d.*eba126e8df00d1cb")
        add_test(NAME victory-view-sod COMMAND wolf3dgeneric-headless
            --data "${WG_TEST_SOD_PATH}" --game SOD --play-view --map 20
            --victory-view --frame-hash --palette-hash)
        set_tests_properties(victory-view-sod PROPERTIES
            PASS_REGULAR_EXPRESSION "dca73a1ba674eeb7.*eba126e8df00d1cb")
        add_test(NAME end-view-sod COMMAND wolf3dgeneric-headless
            --data "${WG_TEST_SOD_PATH}" --game SOD --play-view
            --end-text-view --end-page 1 --frame-hash --palette-hash)
        set_tests_properties(end-view-sod PROPERTIES
            PASS_REGULAR_EXPRESSION "a61316452da19e48.*af03d438783b33e8")
        add_test(NAME high-score-view-sod COMMAND wolf3dgeneric-headless
            --data "${WG_TEST_SOD_PATH}" --game SOD --play-view
            --high-score-view --frame-hash --palette-hash)
        set_tests_properties(high-score-view-sod PROPERTIES
            PASS_REGULAR_EXPRESSION "27c6ea7dee4ac784.*eba126e8df00d1cb")
        add_test(NAME demo-view-sod COMMAND wolf3dgeneric-headless
            --data "${WG_TEST_SOD_PATH}" --game SOD --play-view
            --demo-view --demo-number 0 --demo-commands 70
            --frame-hash --palette-hash)
        set_tests_properties(demo-view-sod PROPERTIES
            PASS_REGULAR_EXPRESSION "fb736a0bff6318ea.*eba126e8df00d1cb")
        set(_wg_sod_demo_hashes
            0e9cf5849373b8f1
            9c558e54d073dfbc
            6e5a538107e55dda
            30509f4a991bbd4d)
        foreach(_wg_demo RANGE 0 3)
            list(GET _wg_sod_demo_hashes ${_wg_demo} _wg_demo_hash)
            add_test(NAME demo-complete-${_wg_demo}-sod
                COMMAND wolf3dgeneric-headless
                    --data "${WG_TEST_SOD_PATH}" --game SOD --play-view
                    --demo-view --demo-number ${_wg_demo}
                    --demo-commands 100000 --frame-hash)
            set_tests_properties(demo-complete-${_wg_demo}-sod PROPERTIES
                PASS_REGULAR_EXPRESSION "${_wg_demo_hash}")
        endforeach()
        add_test(NAME credits-view-sod COMMAND wolf3dgeneric-headless
            --data "${WG_TEST_SOD_PATH}" --game SOD --play-view
            --credits-view --frame-hash --palette-hash)
        set_tests_properties(credits-view-sod PROPERTIES
            PASS_REGULAR_EXPRESSION "9a2558b3b98c49d0.*eba126e8df00d1cb")
    endif()
    if(WG_TEST_SDM_PATH)
        add_test(NAME rating-view-sdm COMMAND wolf3dgeneric-headless
            --data "${WG_TEST_SDM_PATH}" --game SDM --rating-view
            --frame-hash --palette-hash)
        set_tests_properties(rating-view-sdm PROPERTIES
            PASS_REGULAR_EXPRESSION "f49d01316c7fdd4c.*eba126e8df00d1cb")
        add_test(NAME data-sdm COMMAND wg-tests --spear-data SDM 666 2
            "${WG_TEST_SDM_PATH}")
        add_test(NAME title-view-sdm COMMAND wolf3dgeneric-headless
            --data "${WG_TEST_SDM_PATH}" --game SDM --frame-hash
            --palette-hash)
        set_tests_properties(title-view-sdm PROPERTIES
            PASS_REGULAR_EXPRESSION "25bdc19785db9240.*e2a15174c649db6a")
        add_test(NAME main-menu-view-sdm COMMAND wolf3dgeneric-headless
            --data "${WG_TEST_SDM_PATH}" --game SDM --play-view
            --main-menu-view --frame-hash --palette-hash)
        set_tests_properties(main-menu-view-sdm PROPERTIES
            PASS_REGULAR_EXPRESSION "74383419d65cb635.*eba126e8df00d1cb")
        add_test(NAME intermission-view-sdm COMMAND wolf3dgeneric-headless
            --data "${WG_TEST_SDM_PATH}" --game SDM --play-view --map 1
            --intermission-view --frame-hash --palette-hash)
        set_tests_properties(intermission-view-sdm PROPERTIES
            PASS_REGULAR_EXPRESSION "18ddf86c6ac57354.*eba126e8df00d1cb")
        add_test(NAME demo-view-sdm COMMAND wolf3dgeneric-headless
            --data "${WG_TEST_SDM_PATH}" --game SDM --play-view
            --demo-view --demo-number 0 --demo-commands 70
            --frame-hash --palette-hash)
        set_tests_properties(demo-view-sdm PROPERTIES
            PASS_REGULAR_EXPRESSION "fb736a0bff6318ea.*eba126e8df00d1cb")
        add_test(NAME demo-complete-0-sdm COMMAND wolf3dgeneric-headless
            --data "${WG_TEST_SDM_PATH}" --game SDM --play-view
            --demo-view --demo-number 0 --demo-commands 100000 --frame-hash)
        set_tests_properties(demo-complete-0-sdm PROPERTIES
            PASS_REGULAR_EXPRESSION "0e9cf5849373b8f1")
        add_test(NAME credits-view-sdm COMMAND wolf3dgeneric-headless
            --data "${WG_TEST_SDM_PATH}" --game SDM --play-view
            --credits-view --frame-hash --palette-hash)
        set_tests_properties(credits-view-sdm PROPERTIES
            PASS_REGULAR_EXPRESSION "9a2558b3b98c49d0.*eba126e8df00d1cb")
        add_test(NAME conclusion-view-sdm COMMAND wolf3dgeneric-headless
            --data "${WG_TEST_SDM_PATH}" --game SDM --play-view --map 1
            --demo-conclusion-view --frame-hash --palette-hash)
        set_tests_properties(conclusion-view-sdm PROPERTIES
            PASS_REGULAR_EXPRESSION "1b5cc36cbfc9c558.*eba126e8df00d1cb")
    endif()

    foreach(_wg_mission IN ITEMS 1 2 3)
        set(_wg_mission_path "WG_TEST_SD${_wg_mission}_PATH")
        if(${_wg_mission_path})
            if(_wg_mission EQUAL 1)
                set(_wg_mission_pages 666)
            else()
                set(_wg_mission_pages 732)
            endif()
            add_test(NAME external_sd${_wg_mission}_archive
                COMMAND wg-tests --spear-data SD${_wg_mission}
                    ${_wg_mission_pages} 21 "${${_wg_mission_path}}")
            add_test(NAME external_sd${_wg_mission}_headless
                COMMAND wolf3dgeneric-headless
                    --data "${${_wg_mission_path}}"
                    --game SD${_wg_mission} --frame-hash)
        endif()
    endforeach()
    if(WG_TEST_SOD_PATH)
        add_test(NAME signon-apogee COMMAND wolf3dgeneric-headless
            --data "${WG_TEST_SOD_PATH}" --game SOD
            --signon apogee --frame-hash --palette-hash)
        set_tests_properties(signon-apogee PROPERTIES
            PASS_REGULAR_EXPRESSION "3fb3317e1326c57d.*1e14f48394b7e6fd")
        add_test(NAME signon-gt COMMAND wolf3dgeneric-headless
            --data "${WG_TEST_SOD_PATH}" --game SOD
            --signon gt --frame-hash --palette-hash)
        set_tests_properties(signon-gt PROPERTIES
            PASS_REGULAR_EXPRESSION "731879e4b4fee85c.*1e14f48394b7e6fd")
        add_test(NAME signon-id COMMAND wolf3dgeneric-headless
            --data "${WG_TEST_SOD_PATH}" --game SOD
            --signon id --frame-hash --palette-hash)
        set_tests_properties(signon-id PROPERTIES
            PASS_REGULAR_EXPRESSION "b8ce2d3bb335efa0.*1e14f48394b7e6fd")
        add_test(NAME signon-activision COMMAND wolf3dgeneric-headless
            --data "${WG_TEST_SOD_PATH}" --game SOD
            --signon activision --frame-hash --palette-hash)
        set_tests_properties(signon-activision PROPERTIES
            PASS_REGULAR_EXPRESSION "d58d3a1e8c4c2d60.*1e14f48394b7e6fd")
        add_test(NAME signon-spear COMMAND wolf3dgeneric-headless
            --data "${WG_TEST_SOD_PATH}" --game SOD
            --signon spear --frame-hash --palette-hash)
        set_tests_properties(signon-spear PROPERTIES
            PASS_REGULAR_EXPRESSION "7e8d5e435a9f23a8.*eba126e8df00d1cb")
    endif()
endif()

wg_distribution_target_info("${CMAKE_CURRENT_SOURCE_DIR}")
