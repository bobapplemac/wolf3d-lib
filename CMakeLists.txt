cmake_minimum_required(VERSION 3.15)

if(POLICY CMP0091)
    cmake_policy(SET CMP0091 NEW)
endif()

project(wolf3dgeneric VERSION 1.2.0 LANGUAGES C)

include(CTest)

configure_file(packaging/WIN32-README.txt.in
               ${CMAKE_CURRENT_BINARY_DIR}/WIN32-README.txt @ONLY)
configure_file(packaging/LIBRARY-README.txt.in
               ${CMAKE_CURRENT_BINARY_DIR}/LIBRARY-README.txt @ONLY)

option(WG_BUILD_HEADLESS "Build the deterministic headless host" ON)
option(WG_BUILD_WIN32 "Build the dependency-free Win32 host" ON)
option(WG_WARNINGS_AS_ERRORS "Treat project warnings as errors" ON)
option(WG_STATIC_MSVC_RUNTIME
    "Link the MSVC runtime statically for self-contained Windows releases" ON)
set(WG_DIST_ROOT "${CMAKE_SOURCE_DIR}/dist" CACHE PATH
    "Directory used by release staging targets")

if(MSVC AND WG_STATIC_MSVC_RUNTIME)
    set(CMAKE_MSVC_RUNTIME_LIBRARY
        "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

if(NOT WIN32)
    set(WG_BUILD_WIN32 OFF CACHE BOOL "Win32 is only available on Windows" FORCE)
endif()

# Keep the LGPL component independently replaceable on every platform.
add_library(nuked-opl3 SHARED third_party/Nuked-OPL3/opl3.c)
set_target_properties(nuked-opl3 PROPERTIES
    OUTPUT_NAME "Nuked-OPL3"
    WINDOWS_EXPORT_ALL_SYMBOLS ON
)
target_include_directories(nuked-opl3 PUBLIC third_party/Nuked-OPL3)
target_compile_features(nuked-opl3 PUBLIC c_std_99)

set(WG_CORE_SOURCES
    src/WG_ASSETS.c
    src/WG_AUDIO.c
    src/WG_CONFIG.c
    src/ID_CA.c
    src/ID_IN.c
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
    src/WG_PLATFORM.c
    src/ID_US_1.c
    src/WL_AGENT.c
    src/WL_ACT1.c
    src/WL_ACT2.c
    src/WL_PLAY.c
    src/WL_STATE.c
    src/WL_DRAW.c
    src/WL_INTER.c
    src/WL_TEXT.c
    src/WG_RENDERER.c
    src/WG_SAVE.c
    src/WG_SIGNON.c
    src/WL_SCALE.c
    src/ID_VL.c
    src/WL_MAIN.c
    src/WL_MENU.c
    src/WOLF3DGENERIC.c
)

add_library(wolf3dgeneric SHARED ${WG_CORE_SOURCES})
set_target_properties(wolf3dgeneric PROPERTIES
    OUTPUT_NAME "wolf3dgeneric"
    C_VISIBILITY_PRESET hidden
    BUILD_RPATH_USE_ORIGIN ON
)
target_compile_definitions(wolf3dgeneric PRIVATE WOLF3DGENERIC_BUILD)

if(WG_BUILD_HEADLESS)
    add_library(wolf3dgeneric-internal STATIC ${WG_CORE_SOURCES})
    target_compile_definitions(wolf3dgeneric-internal
        PUBLIC WOLF3DGENERIC_STATIC)
endif()

function(wg_configure_core target)
    target_link_libraries(${target} PRIVATE nuked-opl3)

    target_include_directories(${target}
        PUBLIC include
        PRIVATE src
    )

    target_compile_features(${target} PUBLIC c_std_99)

    if(NOT MSVC)
        target_link_libraries(${target} PRIVATE m)
    endif()

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

wg_configure_core(wolf3dgeneric)
if(WG_BUILD_HEADLESS)
    wg_configure_core(wolf3dgeneric-internal)
endif()

if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(WG_RELEASE_ARCH x64)
else()
    set(WG_RELEASE_ARCH x86)
endif()
string(TOLOWER "${CMAKE_SYSTEM_NAME}" WG_RELEASE_SYSTEM)
set(WG_LIBRARY_RELEASE_DIR
    "${WG_DIST_ROOT}/wolf3dgeneric-${PROJECT_VERSION}-library-${WG_RELEASE_SYSTEM}-${WG_RELEASE_ARCH}")

set(WG_LIBRARY_RELEASE_COMMANDS
    COMMAND ${CMAKE_COMMAND} -E remove_directory
        "${WG_LIBRARY_RELEASE_DIR}"
    COMMAND ${CMAKE_COMMAND} -E make_directory
        "${WG_LIBRARY_RELEASE_DIR}"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "$<TARGET_FILE:wolf3dgeneric>"
        "${WG_LIBRARY_RELEASE_DIR}/$<TARGET_FILE_NAME:wolf3dgeneric>"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "$<TARGET_FILE:nuked-opl3>"
        "${WG_LIBRARY_RELEASE_DIR}/$<TARGET_FILE_NAME:nuked-opl3>"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_SOURCE_DIR}/include/WOLF3DGENERIC.h"
        "${WG_LIBRARY_RELEASE_DIR}/WOLF3DGENERIC.h"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_SOURCE_DIR}/LICENSE"
        "${WG_LIBRARY_RELEASE_DIR}/COPYING.txt"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_SOURCE_DIR}/third_party/Nuked-OPL3/LICENSE"
        "${WG_LIBRARY_RELEASE_DIR}/COPYING.Nuked-OPL3.txt"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_SOURCE_DIR}/THIRD_PARTY.md"
        "${WG_LIBRARY_RELEASE_DIR}/THIRD_PARTY.md"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_BINARY_DIR}/LIBRARY-README.txt"
        "${WG_LIBRARY_RELEASE_DIR}/README.txt"
)
if(WIN32)
    list(APPEND WG_LIBRARY_RELEASE_COMMANDS
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "$<TARGET_LINKER_FILE:wolf3dgeneric>"
            "${WG_LIBRARY_RELEASE_DIR}/$<TARGET_LINKER_FILE_NAME:wolf3dgeneric>"
    )
endif()
add_custom_target(library-release
    ${WG_LIBRARY_RELEASE_COMMANDS}
    DEPENDS wolf3dgeneric nuked-opl3
    COMMENT "Staging ${WG_RELEASE_ARCH} wolf3dgeneric shared library"
    VERBATIM
)

function(wg_configure_host target engine_target)
    target_link_libraries(${target} PRIVATE ${engine_target})
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
    wg_configure_host(wolf3dgeneric-headless wolf3dgeneric-internal)
endif()

if(WG_BUILD_WIN32)
    add_executable(wolf3dgeneric-win32 WIN32
        platforms/win32/WG_WIN32.c
    )
    wg_configure_host(wolf3dgeneric-win32 wolf3dgeneric)
    target_compile_definitions(wolf3dgeneric-win32 PRIVATE
        WIN32_LEAN_AND_MEAN
        UNICODE
        _UNICODE
    )
    target_link_libraries(wolf3dgeneric-win32 PRIVATE user32 gdi32 winmm shell32)

    set(WG_WIN32_RELEASE_DIR
        "${WG_DIST_ROOT}/wolf3dgeneric-${PROJECT_VERSION}-win32-${WG_RELEASE_ARCH}")
    add_custom_target(win32-release
        COMMAND ${CMAKE_COMMAND} -E remove_directory
            "${WG_WIN32_RELEASE_DIR}"
        COMMAND ${CMAKE_COMMAND} -E make_directory
            "${WG_WIN32_RELEASE_DIR}"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "$<TARGET_FILE:wolf3dgeneric-win32>"
            "${WG_WIN32_RELEASE_DIR}/wolf3dgeneric.exe"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "$<TARGET_FILE:wolf3dgeneric>"
            "${WG_WIN32_RELEASE_DIR}/wolf3dgeneric.dll"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "$<TARGET_FILE:nuked-opl3>"
            "${WG_WIN32_RELEASE_DIR}/Nuked-OPL3.dll"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_SOURCE_DIR}/LICENSE"
            "${WG_WIN32_RELEASE_DIR}/COPYING.txt"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_SOURCE_DIR}/third_party/Nuked-OPL3/LICENSE"
            "${WG_WIN32_RELEASE_DIR}/COPYING.Nuked-OPL3.txt"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_SOURCE_DIR}/THIRD_PARTY.md"
            "${WG_WIN32_RELEASE_DIR}/THIRD_PARTY.md"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_CURRENT_BINARY_DIR}/WIN32-README.txt"
            "${WG_WIN32_RELEASE_DIR}/README.txt"
        DEPENDS wolf3dgeneric-win32 wolf3dgeneric nuked-opl3
        COMMENT "Staging ${WG_RELEASE_ARCH} Win32 release folder"
        VERBATIM
    )
endif()

if(BUILD_TESTING AND WG_BUILD_HEADLESS)
    add_test(NAME headless-smoke COMMAND wolf3dgeneric-headless --headless-smoke)

    add_executable(wg-tests tests/WG_TESTS.c)
    wg_configure_host(wg-tests wolf3dgeneric-internal)
    add_test(NAME unit-tests COMMAND wg-tests)

    set(WG_TEST_WL1_PATH "" CACHE PATH "Path to external Wolf3D v1.4 WL1 data")
    set(WG_TEST_WL6_PATH "" CACHE PATH "Path to external Wolf3D v1.4 WL6 data")
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
            PASS_REGULAR_EXPRESSION "88648019bc43feb5.*1e14f48394b7e6fd")
        add_test(NAME rating-view-wl1 COMMAND wolf3dgeneric-headless
            --data "${WG_TEST_WL1_PATH}" --game WL1 --rating-view
            --frame-hash --palette-hash)
        set_tests_properties(rating-view-wl1 PROPERTIES
            PASS_REGULAR_EXPRESSION "f49d01316c7fdd4c.*1e14f48394b7e6fd")
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
        add_test(NAME pause-view-wl1 COMMAND wolf3dgeneric-headless
            --data "${WG_TEST_WL1_PATH}" --play-view --pause-view
            --frame-hash)
        set_tests_properties(pause-view-wl1 PROPERTIES
            PASS_REGULAR_EXPRESSION "fb088c39d6f75570")
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
            PASS_REGULAR_EXPRESSION "110c844dbc2a3366")
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
            PASS_REGULAR_EXPRESSION "a0188da801550518")
        add_test(NAME save-game-view-wl1 COMMAND wolf3dgeneric-headless
            --data "${WG_TEST_WL1_PATH}" --play-view --save-game-view
            --frame-hash)
        set_tests_properties(save-game-view-wl1 PROPERTIES
            PASS_REGULAR_EXPRESSION "d81cecaa1eef0887")
        add_test(NAME sound-menu-view-wl1 COMMAND wolf3dgeneric-headless
            --data "${WG_TEST_WL1_PATH}" --play-view --sound-menu-view
            --frame-hash)
        set_tests_properties(sound-menu-view-wl1 PROPERTIES
            PASS_REGULAR_EXPRESSION "f46530fbaa7a0775")
        add_test(NAME control-menu-view-wl1 COMMAND wolf3dgeneric-headless
            --data "${WG_TEST_WL1_PATH}" --play-view --control-menu-view
            --frame-hash)
        set_tests_properties(control-menu-view-wl1 PROPERTIES
            PASS_REGULAR_EXPRESSION "6b0f54c32a663fa5")
        add_test(NAME joystick-menu-view-wl1 COMMAND wolf3dgeneric-headless
            --data "${WG_TEST_WL1_PATH}" --play-view --joystick-menu-view
            --frame-hash)
        set_tests_properties(joystick-menu-view-wl1 PROPERTIES
            PASS_REGULAR_EXPRESSION "6a7611870fce1ab5")
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
            PASS_REGULAR_EXPRESSION "f94ac3381beb5e83")
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
            PASS_REGULAR_EXPRESSION "063d5289c4c1414e")
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
            PASS_REGULAR_EXPRESSION "10458addd72fb2c0")
        add_test(NAME demo-complete-0-wl1 COMMAND wolf3dgeneric-headless
            --data "${WG_TEST_WL1_PATH}" --play-view --demo-view
            --demo-number 0 --demo-commands 1152 --frame-hash)
        set_tests_properties(demo-complete-0-wl1 PROPERTIES
            PASS_REGULAR_EXPRESSION "5b816ab21404372a")
        add_test(NAME demo-complete-1-wl1 COMMAND wolf3dgeneric-headless
            --data "${WG_TEST_WL1_PATH}" --play-view --demo-view
            --demo-number 1 --demo-commands 1284 --frame-hash)
        set_tests_properties(demo-complete-1-wl1 PROPERTIES
            PASS_REGULAR_EXPRESSION "d0abd2087b49a774")
        add_test(NAME demo-complete-2-wl1 COMMAND wolf3dgeneric-headless
            --data "${WG_TEST_WL1_PATH}" --play-view --demo-view
            --demo-number 2 --demo-commands 671 --frame-hash)
        set_tests_properties(demo-complete-2-wl1 PROPERTIES
            PASS_REGULAR_EXPRESSION "0b21e54546425858")
        add_test(NAME demo-complete-3-wl1 COMMAND wolf3dgeneric-headless
            --data "${WG_TEST_WL1_PATH}" --play-view --demo-view
            --demo-number 3 --demo-commands 633 --frame-hash)
        set_tests_properties(demo-complete-3-wl1 PROPERTIES
            PASS_REGULAR_EXPRESSION "3829f1bd054113c9")
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
    if(WG_TEST_WL6_PATH)
        add_test(NAME data-wl6 COMMAND wg-tests --data wl6 "${WG_TEST_WL6_PATH}")
        add_test(NAME signon-auto-wl6 COMMAND wolf3dgeneric-headless
            --data "${WG_TEST_WL6_PATH}" --game WL6 --signon auto
            --frame-hash --palette-hash)
        set_tests_properties(signon-auto-wl6 PROPERTIES
            PASS_REGULAR_EXPRESSION "fa8a46047d53d714.*1e14f48394b7e6fd")
        add_test(NAME rating-view-wl6 COMMAND wolf3dgeneric-headless
            --data "${WG_TEST_WL6_PATH}" --game WL6 --rating-view
            --frame-hash --palette-hash)
        set_tests_properties(rating-view-wl6 PROPERTIES
            PASS_REGULAR_EXPRESSION "f49d01316c7fdd4c.*1e14f48394b7e6fd")
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
        add_test(NAME pause-view-wl6 COMMAND wolf3dgeneric-headless
            --data "${WG_TEST_WL6_PATH}" --play-view --pause-view
            --frame-hash)
        set_tests_properties(pause-view-wl6 PROPERTIES
            PASS_REGULAR_EXPRESSION "fb088c39d6f75570")
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
            PASS_REGULAR_EXPRESSION "110c844dbc2a3366")
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
            PASS_REGULAR_EXPRESSION "cdbff8b31548b64e")
        add_test(NAME in-game-menu-view-wl6 COMMAND wolf3dgeneric-headless
            --data "${WG_TEST_WL6_PATH}" --play-view --in-game-menu-view
            --frame-hash)
        set_tests_properties(in-game-menu-view-wl6 PROPERTIES
            PASS_REGULAR_EXPRESSION "9d4fae41d9d29488")
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
            PASS_REGULAR_EXPRESSION "466d386e2915c931")
        add_test(NAME joystick-menu-view-wl6 COMMAND wolf3dgeneric-headless
            --data "${WG_TEST_WL6_PATH}" --play-view --joystick-menu-view
            --frame-hash)
        set_tests_properties(joystick-menu-view-wl6 PROPERTIES
            PASS_REGULAR_EXPRESSION "07dc46006dc97b99")
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
            PASS_REGULAR_EXPRESSION "2ac0b2270dc20b66")
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
            PASS_REGULAR_EXPRESSION "063d5289c4c1414e")
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
            PASS_REGULAR_EXPRESSION "8f5dc9097c041505")
        add_test(NAME demo-complete-0-wl6 COMMAND wolf3dgeneric-headless
            --data "${WG_TEST_WL6_PATH}" --play-view --demo-view
            --demo-number 0 --demo-commands 691 --frame-hash)
        set_tests_properties(demo-complete-0-wl6 PROPERTIES
            PASS_REGULAR_EXPRESSION "8b78f328d66287ab")
        add_test(NAME demo-complete-1-wl6 COMMAND wolf3dgeneric-headless
            --data "${WG_TEST_WL6_PATH}" --play-view --demo-view
            --demo-number 1 --demo-commands 1899 --frame-hash)
        set_tests_properties(demo-complete-1-wl6 PROPERTIES
            PASS_REGULAR_EXPRESSION "4cc5578ebc5d9cfc")
        add_test(NAME demo-complete-2-wl6 COMMAND wolf3dgeneric-headless
            --data "${WG_TEST_WL6_PATH}" --play-view --demo-view
            --demo-number 2 --demo-commands 1140 --frame-hash)
        set_tests_properties(demo-complete-2-wl6 PROPERTIES
            PASS_REGULAR_EXPRESSION "90df6b099486f7b1")
        add_test(NAME demo-complete-3-wl6 COMMAND wolf3dgeneric-headless
            --data "${WG_TEST_WL6_PATH}" --play-view --demo-view
            --demo-number 3 --demo-commands 1656 --frame-hash)
        set_tests_properties(demo-complete-3-wl6 PROPERTIES
            PASS_REGULAR_EXPRESSION "c32e04314c8181d4")
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
    if(WG_TEST_SOD_PATH)
        add_test(NAME signon-auto-sod COMMAND wolf3dgeneric-headless
            --data "${WG_TEST_SOD_PATH}" --game SOD --signon auto
            --frame-hash --palette-hash)
        set_tests_properties(signon-auto-sod PROPERTIES
            PASS_REGULAR_EXPRESSION "9e9b215a63203250.*eba126e8df00d1cb")
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
            PASS_REGULAR_EXPRESSION "ec2699111d9f0299.*eba126e8df00d1cb")
        add_test(NAME difficulty-menu-view-sod COMMAND wolf3dgeneric-headless
            --data "${WG_TEST_SOD_PATH}" --game SOD --play-view
            --difficulty-menu-view --frame-hash --palette-hash)
        set_tests_properties(difficulty-menu-view-sod PROPERTIES
            PASS_REGULAR_EXPRESSION "4b9dd46549814400.*eba126e8df00d1cb")
        add_test(NAME trans-view-sod COMMAND wolf3dgeneric-headless
            --data "${WG_TEST_SOD_PATH}" --game SOD --play-view --map 4
            --boss-view --frame-hash --palette-hash)
        set_tests_properties(trans-view-sod PROPERTIES
            PASS_REGULAR_EXPRESSION "dd1b3992157d8354.*eba126e8df00d1cb")
        add_test(NAME intermission-view-sod COMMAND wolf3dgeneric-headless
            --data "${WG_TEST_SOD_PATH}" --game SOD --play-view --map 4
            --intermission-view --frame-hash --palette-hash)
        set_tests_properties(intermission-view-sod PROPERTIES
            PASS_REGULAR_EXPRESSION "9cc20a4c95361c33.*eba126e8df00d1cb")
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
            PASS_REGULAR_EXPRESSION "036fbc4175dd12ab.*eba126e8df00d1cb")
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
            PASS_REGULAR_EXPRESSION "525cbfb8e59bee04.*eba126e8df00d1cb")
        set(_wg_sod_demo_hashes
            1b1413cf97abe5ad
            015b8d5e3c3a84fc
            b46ad48632050ce8
            1c531d2ea19774a2)
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
            PASS_REGULAR_EXPRESSION "ec2699111d9f0299.*eba126e8df00d1cb")
        add_test(NAME intermission-view-sdm COMMAND wolf3dgeneric-headless
            --data "${WG_TEST_SDM_PATH}" --game SDM --play-view --map 1
            --intermission-view --frame-hash --palette-hash)
        set_tests_properties(intermission-view-sdm PROPERTIES
            PASS_REGULAR_EXPRESSION "4b48bd845813fdf5.*eba126e8df00d1cb")
        add_test(NAME demo-view-sdm COMMAND wolf3dgeneric-headless
            --data "${WG_TEST_SDM_PATH}" --game SDM --play-view
            --demo-view --demo-number 0 --demo-commands 70
            --frame-hash --palette-hash)
        set_tests_properties(demo-view-sdm PROPERTIES
            PASS_REGULAR_EXPRESSION "525cbfb8e59bee04.*eba126e8df00d1cb")
        add_test(NAME demo-complete-0-sdm COMMAND wolf3dgeneric-headless
            --data "${WG_TEST_SDM_PATH}" --game SDM --play-view
            --demo-view --demo-number 0 --demo-commands 100000 --frame-hash)
        set_tests_properties(demo-complete-0-sdm PROPERTIES
            PASS_REGULAR_EXPRESSION "1b1413cf97abe5ad")
        add_test(NAME credits-view-sdm COMMAND wolf3dgeneric-headless
            --data "${WG_TEST_SDM_PATH}" --game SDM --play-view
            --credits-view --frame-hash --palette-hash)
        set_tests_properties(credits-view-sdm PROPERTIES
            PASS_REGULAR_EXPRESSION "9a2558b3b98c49d0.*eba126e8df00d1cb")
        add_test(NAME conclusion-view-sdm COMMAND wolf3dgeneric-headless
            --data "${WG_TEST_SDM_PATH}" --game SDM --play-view --map 1
            --demo-conclusion-view --frame-hash --palette-hash)
        set_tests_properties(conclusion-view-sdm PROPERTIES
            PASS_REGULAR_EXPRESSION "e8186a081eb15e49.*eba126e8df00d1cb")
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
            PASS_REGULAR_EXPRESSION "88648019bc43feb5.*1e14f48394b7e6fd")
        add_test(NAME signon-gt COMMAND wolf3dgeneric-headless
            --data "${WG_TEST_SOD_PATH}" --game SOD
            --signon gt --frame-hash --palette-hash)
        set_tests_properties(signon-gt PROPERTIES
            PASS_REGULAR_EXPRESSION "fa8a46047d53d714.*1e14f48394b7e6fd")
        add_test(NAME signon-id COMMAND wolf3dgeneric-headless
            --data "${WG_TEST_SOD_PATH}" --game SOD
            --signon id --frame-hash --palette-hash)
        set_tests_properties(signon-id PROPERTIES
            PASS_REGULAR_EXPRESSION "ae7cd1709e0f8718.*1e14f48394b7e6fd")
        add_test(NAME signon-activision COMMAND wolf3dgeneric-headless
            --data "${WG_TEST_SOD_PATH}" --game SOD
            --signon activision --frame-hash --palette-hash)
        set_tests_properties(signon-activision PROPERTIES
            PASS_REGULAR_EXPRESSION "138525e53eec7ed8.*1e14f48394b7e6fd")
        add_test(NAME signon-spear COMMAND wolf3dgeneric-headless
            --data "${WG_TEST_SOD_PATH}" --game SOD
            --signon spear --frame-hash --palette-hash)
        set_tests_properties(signon-spear PROPERTIES
            PASS_REGULAR_EXPRESSION "9e9b215a63203250.*eba126e8df00d1cb")
    endif()
endif()
