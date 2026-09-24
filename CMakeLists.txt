cmake_minimum_required(VERSION 3.15)

project(wolf3dgeneric VERSION 0.1.0 LANGUAGES C)

include(CTest)

option(WG_BUILD_HEADLESS "Build the deterministic headless host" ON)
option(WG_BUILD_WIN32 "Build the dependency-free Win32 host" ON)
option(WG_WARNINGS_AS_ERRORS "Treat project warnings as errors" ON)

if(NOT WIN32)
    set(WG_BUILD_WIN32 OFF CACHE BOOL "Win32 is only available on Windows" FORCE)
endif()

add_library(wolf3dgeneric STATIC
    src/wg_assets.c
    src/wg_audio.c
    src/wg_compression.c
    src/wg_data.c
    src/wg_file.c
    src/wg_font.c
    src/wg_graphics.c
    src/wg_level.c
    src/wg_maps.c
    src/wg_pages.c
    src/wg_palette.c
    src/wg_random.c
    src/wg_video.c
    src/wolf3dgeneric.c
)

target_include_directories(wolf3dgeneric
    PUBLIC include
    PRIVATE src
)

target_compile_features(wolf3dgeneric PUBLIC c_std_99)

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
        platforms/headless/main.c
        platforms/headless/wg_headless.c
    )
    wg_configure_host(wolf3dgeneric-headless)
endif()

if(WG_BUILD_WIN32)
    add_executable(wolf3dgeneric-win32 WIN32
        platforms/win32/wg_win32.c
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

    add_executable(wg-tests tests/wg_tests.c)
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
