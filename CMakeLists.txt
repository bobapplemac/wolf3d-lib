cmake_minimum_required(VERSION 3.15)

project(wolf3dgeneric VERSION 0.1.0 LANGUAGES C)

include(CTest)

option(WG_BUILD_HEADLESS "Build the deterministic headless host" ON)
option(WG_BUILD_WIN32 "Build the dependency-free Win32 host" ON)
option(WG_WARNINGS_AS_ERRORS "Treat project warnings as errors" ON)

if(NOT WIN32)
    set(WG_BUILD_WIN32 OFF CACHE BOOL "Win32 is only available on Windows" FORCE)
endif()

set(WG_CORE_SOURCES
    src/WG_ASSETS.c
    src/WG_AUDIO.c
    src/ID_CA.c
    src/WG_DATA.c
    src/WG_FILE.c
    src/WG_FIXED.c
    src/ID_VH.c
    src/WG_GRAPHICS.c
    src/WL_GAME.c
    src/WG_MAPS.c
    src/ID_PM.c
    src/WG_PALETTE.c
    src/ID_US_1.c
    src/WL_AGENT.c
    src/WL_DRAW.c
    src/WG_RENDERER.c
    src/WL_SCALE.c
    src/ID_VL.c
    src/WL_MAIN.c
    src/WOLF3DGENERIC.c
)

add_library(wolf3dgeneric STATIC ${WG_CORE_SOURCES})

target_include_directories(wolf3dgeneric
    PUBLIC include
    PRIVATE src
)

target_compile_features(wolf3dgeneric PUBLIC c_std_99)

if(NOT MSVC)
    target_link_libraries(wolf3dgeneric PUBLIC m)
endif()

if(MSVC)
    target_compile_options(wolf3dgeneric PRIVATE /W4 /permissive-)
    if(WG_WARNINGS_AS_ERRORS)
        target_compile_options(wolf3dgeneric PRIVATE /WX)
    endif()
else()
    target_compile_options(wolf3dgeneric PRIVATE -Wall -Wextra -Wpedantic)
    if(WG_WARNINGS_AS_ERRORS)
        target_compile_options(wolf3dgeneric PRIVATE -Werror)
    endif()
endif()

function(wg_configure_host target)
    target_link_libraries(${target} PRIVATE wolf3dgeneric)
    target_include_directories(${target} PRIVATE src)
    target_compile_features(${target} PRIVATE c_std_99)

    if(MSVC)
        target_compile_options(${target} PRIVATE /W4 /permissive-)
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
        platforms/headless/MAIN.c
        platforms/headless/WG_HEADLESS.c
    )
    wg_configure_host(wolf3dgeneric-headless)
endif()

if(WG_BUILD_WIN32)
    add_executable(wolf3dgeneric-win32 WIN32
        platforms/win32/WG_WIN32.c
    )
    wg_configure_host(wolf3dgeneric-win32)
    target_compile_definitions(wolf3dgeneric-win32 PRIVATE
        WIN32_LEAN_AND_MEAN
        UNICODE
        _UNICODE
    )
    target_link_libraries(wolf3dgeneric-win32 PRIVATE user32 gdi32 winmm shell32)
endif()

if(BUILD_TESTING AND WG_BUILD_HEADLESS)
    add_test(NAME bootstrap-headless COMMAND wolf3dgeneric-headless --bootstrap-test)

    add_executable(wg-tests tests/WG_TESTS.c)
    wg_configure_host(wg-tests)
    add_test(NAME unit-tests COMMAND wg-tests)

    set(WG_TEST_WL1_PATH "" CACHE PATH "Path to external Wolf3D v1.4 WL1 data")
    set(WG_TEST_WL6_PATH "" CACHE PATH "Path to external Wolf3D v1.4 WL6 data")
    if(WG_TEST_WL1_PATH)
        add_test(NAME data-wl1 COMMAND wg-tests --data wl1 "${WG_TEST_WL1_PATH}")
    endif()
    if(WG_TEST_WL6_PATH)
        add_test(NAME data-wl6 COMMAND wg-tests --data wl6 "${WG_TEST_WL6_PATH}")
    endif()
endif()
