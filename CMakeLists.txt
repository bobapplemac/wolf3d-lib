cmake_minimum_required(VERSION 3.15)

project(wolf3dgeneric VERSION 0.1.0 LANGUAGES C)

include(CTest)

option(WG_BUILD_HEADLESS "Build the deterministic headless host" ON)
option(WG_BUILD_WIN32 "Build the dependency-free Win32 host" ON)
option(WG_WARNINGS_AS_ERRORS "Treat project warnings as errors" ON)

if(NOT WIN32)
    set(WG_BUILD_WIN32 OFF CACHE BOOL "Win32 is only available on Windows" FORCE)
endif()

# Kept as a separate LGPL component so its upstream source and replacement
# boundary remain independently reviewable.
add_library(nuked-opl3 STATIC third_party/Nuked-OPL3/opl3.c)
target_include_directories(nuked-opl3 PUBLIC third_party/Nuked-OPL3)
target_compile_features(nuked-opl3 PUBLIC c_std_99)

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
    src/ID_SD.c
    src/WG_PALETTE.c
    src/ID_US_1.c
    src/WL_AGENT.c
    src/WL_ACT1.c
    src/WL_ACT2.c
    src/WL_PLAY.c
    src/WL_STATE.c
    src/WL_DRAW.c
    src/WG_RENDERER.c
    src/WL_SCALE.c
    src/ID_VL.c
    src/WL_MAIN.c
    src/WOLF3DGENERIC.c
)

add_library(wolf3dgeneric STATIC ${WG_CORE_SOURCES})

target_link_libraries(wolf3dgeneric PUBLIC nuked-opl3)

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
        add_test(NAME boss-view-wl1 COMMAND wolf3dgeneric-headless
            --data "${WG_TEST_WL1_PATH}" --play-view --map 8 --boss-view
            --frame-hash)
        set_tests_properties(boss-view-wl1 PROPERTIES
            PASS_REGULAR_EXPRESSION "a38a57b87c979508")
        add_test(NAME boss-fire-view-wl1 COMMAND wolf3dgeneric-headless
            --data "${WG_TEST_WL1_PATH}" --play-view --map 8
            --boss-fire-view --frame-hash)
        set_tests_properties(boss-fire-view-wl1 PROPERTIES
            PASS_REGULAR_EXPRESSION "cd8a4e5001298ee6")
        add_test(NAME patrol-view-wl1 COMMAND wolf3dgeneric-headless
            --data "${WG_TEST_WL1_PATH}" --play-view --open-doors
            --patrol-view --actor-tics 127 --frame-hash)
        set_tests_properties(patrol-view-wl1 PROPERTIES
            PASS_REGULAR_EXPRESSION "0460cc1c73d44f60")
        add_test(NAME alert-view-wl1 COMMAND wolf3dgeneric-headless
            --data "${WG_TEST_WL1_PATH}" --play-view --alert-view
            --frame-hash)
        set_tests_properties(alert-view-wl1 PROPERTIES
            PASS_REGULAR_EXPRESSION "63df8fa451a6ba6b")
        add_test(NAME chase-view-wl1 COMMAND wolf3dgeneric-headless
            --data "${WG_TEST_WL1_PATH}" --play-view --chase-view
            --actor-tics 9 --frame-hash)
        set_tests_properties(chase-view-wl1 PROPERTIES
            PASS_REGULAR_EXPRESSION "8485e203e6d30079")
        add_test(NAME fire-view-wl1 COMMAND wolf3dgeneric-headless
            --data "${WG_TEST_WL1_PATH}" --play-view --fire-view
            --frame-hash)
        set_tests_properties(fire-view-wl1 PROPERTIES
            PASS_REGULAR_EXPRESSION "dbfa84e64bb68acf")
        add_test(NAME bite-view-wl1 COMMAND wolf3dgeneric-headless
            --data "${WG_TEST_WL1_PATH}" --play-view --bite-view
            --frame-hash)
        set_tests_properties(bite-view-wl1 PROPERTIES
            PASS_REGULAR_EXPRESSION "2391755b562c8744")
        add_test(NAME pushwall-view-wl1 COMMAND wolf3dgeneric-headless
            --data "${WG_TEST_WL1_PATH}" --play-view --pushwall-view
            --frame-hash)
        set_tests_properties(pushwall-view-wl1 PROPERTIES
            PASS_REGULAR_EXPRESSION "48bdacd231ece2d9")
        add_test(NAME death-view-wl1 COMMAND wolf3dgeneric-headless
            --data "${WG_TEST_WL1_PATH}" --play-view --death-view
            --frame-hash)
        set_tests_properties(death-view-wl1 PROPERTIES
            PASS_REGULAR_EXPRESSION "1a6b2675d0e3e31b")
        add_test(NAME boss-death-view-wl1 COMMAND wolf3dgeneric-headless
            --data "${WG_TEST_WL1_PATH}" --play-view --map 8
            --boss-death-view --frame-hash)
        set_tests_properties(boss-death-view-wl1 PROPERTIES
            PASS_REGULAR_EXPRESSION "2a5b6e8514c2298b")
        add_test(NAME player-fire-view-wl1 COMMAND wolf3dgeneric-headless
            --data "${WG_TEST_WL1_PATH}" --play-view --player-fire-view
            --frame-hash)
        set_tests_properties(player-fire-view-wl1 PROPERTIES
            PASS_REGULAR_EXPRESSION "6fb9a7201588ef0c")
        add_test(NAME pickup-view-wl1 COMMAND wolf3dgeneric-headless
            --data "${WG_TEST_WL1_PATH}" --play-view --pickup-view
            --frame-hash)
        set_tests_properties(pickup-view-wl1 PROPERTIES
            PASS_REGULAR_EXPRESSION "3745f8af67b60f69")
        add_test(NAME door-use-view-wl1 COMMAND wolf3dgeneric-headless
            --data "${WG_TEST_WL1_PATH}" --play-view --door-use-view
            --frame-hash)
        set_tests_properties(door-use-view-wl1 PROPERTIES
            PASS_REGULAR_EXPRESSION "d5720a5ad520646d")
        add_test(NAME play-loop-forward-wl1 COMMAND wolf3dgeneric-headless
            --data "${WG_TEST_WL1_PATH}" --play-view --forward-tics 35
            --frame-hash)
        set_tests_properties(play-loop-forward-wl1 PROPERTIES
            PASS_REGULAR_EXPRESSION "41b5819e00a95446")
        add_test(NAME next-floor-view-wl1 COMMAND wolf3dgeneric-headless
            --data "${WG_TEST_WL1_PATH}" --play-view --map 1
            --frame-hash)
        set_tests_properties(next-floor-view-wl1 PROPERTIES
            PASS_REGULAR_EXPRESSION "dd20149a5592b546")
    endif()
    if(WG_TEST_WL6_PATH)
        add_test(NAME data-wl6 COMMAND wg-tests --data wl6 "${WG_TEST_WL6_PATH}")
        add_test(NAME boss-view-wl6 COMMAND wolf3dgeneric-headless
            --data "${WG_TEST_WL6_PATH}" --play-view --map 8 --boss-view
            --frame-hash)
        set_tests_properties(boss-view-wl6 PROPERTIES
            PASS_REGULAR_EXPRESSION "a38a57b87c979508")
        add_test(NAME boss-fire-view-wl6 COMMAND wolf3dgeneric-headless
            --data "${WG_TEST_WL6_PATH}" --play-view --map 8
            --boss-fire-view --frame-hash)
        set_tests_properties(boss-fire-view-wl6 PROPERTIES
            PASS_REGULAR_EXPRESSION "cd8a4e5001298ee6")
        add_test(NAME patrol-view-wl6 COMMAND wolf3dgeneric-headless
            --data "${WG_TEST_WL6_PATH}" --play-view --open-doors
            --patrol-view --actor-tics 127 --frame-hash)
        set_tests_properties(patrol-view-wl6 PROPERTIES
            PASS_REGULAR_EXPRESSION "0460cc1c73d44f60")
        add_test(NAME alert-view-wl6 COMMAND wolf3dgeneric-headless
            --data "${WG_TEST_WL6_PATH}" --play-view --alert-view
            --frame-hash)
        set_tests_properties(alert-view-wl6 PROPERTIES
            PASS_REGULAR_EXPRESSION "63df8fa451a6ba6b")
        add_test(NAME chase-view-wl6 COMMAND wolf3dgeneric-headless
            --data "${WG_TEST_WL6_PATH}" --play-view --chase-view
            --actor-tics 9 --frame-hash)
        set_tests_properties(chase-view-wl6 PROPERTIES
            PASS_REGULAR_EXPRESSION "8485e203e6d30079")
        add_test(NAME fire-view-wl6 COMMAND wolf3dgeneric-headless
            --data "${WG_TEST_WL6_PATH}" --play-view --fire-view
            --frame-hash)
        set_tests_properties(fire-view-wl6 PROPERTIES
            PASS_REGULAR_EXPRESSION "dbfa84e64bb68acf")
        add_test(NAME bite-view-wl6 COMMAND wolf3dgeneric-headless
            --data "${WG_TEST_WL6_PATH}" --play-view --bite-view
            --frame-hash)
        set_tests_properties(bite-view-wl6 PROPERTIES
            PASS_REGULAR_EXPRESSION "2391755b562c8744")
        add_test(NAME needle-view-wl6 COMMAND wolf3dgeneric-headless
            --data "${WG_TEST_WL6_PATH}" --play-view --map 18
            --needle-view --frame-hash)
        set_tests_properties(needle-view-wl6 PROPERTIES
            PASS_REGULAR_EXPRESSION "581ce5d3716e7ca0")
        add_test(NAME rocket-view-wl6 COMMAND wolf3dgeneric-headless
            --data "${WG_TEST_WL6_PATH}" --play-view --map 38
            --rocket-view --frame-hash)
        set_tests_properties(rocket-view-wl6 PROPERTIES
            PASS_REGULAR_EXPRESSION "c85b4f4095ac18f2")
        add_test(NAME flame-view-wl6 COMMAND wolf3dgeneric-headless
            --data "${WG_TEST_WL6_PATH}" --play-view --map 28
            --flame-view --frame-hash)
        set_tests_properties(flame-view-wl6 PROPERTIES
            PASS_REGULAR_EXPRESSION "ba572ca325d89701")
        add_test(NAME pushwall-view-wl6 COMMAND wolf3dgeneric-headless
            --data "${WG_TEST_WL6_PATH}" --play-view --pushwall-view
            --frame-hash)
        set_tests_properties(pushwall-view-wl6 PROPERTIES
            PASS_REGULAR_EXPRESSION "48bdacd231ece2d9")
        add_test(NAME death-view-wl6 COMMAND wolf3dgeneric-headless
            --data "${WG_TEST_WL6_PATH}" --play-view --death-view
            --frame-hash)
        set_tests_properties(death-view-wl6 PROPERTIES
            PASS_REGULAR_EXPRESSION "1a6b2675d0e3e31b")
        add_test(NAME boss-death-view-wl6 COMMAND wolf3dgeneric-headless
            --data "${WG_TEST_WL6_PATH}" --play-view --map 8
            --boss-death-view --frame-hash)
        set_tests_properties(boss-death-view-wl6 PROPERTIES
            PASS_REGULAR_EXPRESSION "2a5b6e8514c2298b")
        add_test(NAME player-fire-view-wl6 COMMAND wolf3dgeneric-headless
            --data "${WG_TEST_WL6_PATH}" --play-view --player-fire-view
            --frame-hash)
        set_tests_properties(player-fire-view-wl6 PROPERTIES
            PASS_REGULAR_EXPRESSION "6fb9a7201588ef0c")
        add_test(NAME pickup-view-wl6 COMMAND wolf3dgeneric-headless
            --data "${WG_TEST_WL6_PATH}" --play-view --pickup-view
            --frame-hash)
        set_tests_properties(pickup-view-wl6 PROPERTIES
            PASS_REGULAR_EXPRESSION "3745f8af67b60f69")
        add_test(NAME door-use-view-wl6 COMMAND wolf3dgeneric-headless
            --data "${WG_TEST_WL6_PATH}" --play-view --door-use-view
            --frame-hash)
        set_tests_properties(door-use-view-wl6 PROPERTIES
            PASS_REGULAR_EXPRESSION "d5720a5ad520646d")
        add_test(NAME play-loop-forward-wl6 COMMAND wolf3dgeneric-headless
            --data "${WG_TEST_WL6_PATH}" --play-view --forward-tics 35
            --frame-hash)
        set_tests_properties(play-loop-forward-wl6 PROPERTIES
            PASS_REGULAR_EXPRESSION "41b5819e00a95446")
        add_test(NAME next-floor-view-wl6 COMMAND wolf3dgeneric-headless
            --data "${WG_TEST_WL6_PATH}" --play-view --map 1
            --frame-hash)
        set_tests_properties(next-floor-view-wl6 PROPERTIES
            PASS_REGULAR_EXPRESSION "dd20149a5592b546")
    endif()
endif()
