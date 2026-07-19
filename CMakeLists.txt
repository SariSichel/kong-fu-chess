cmake_minimum_required(VERSION 3.16)
project(kong_fu_chess LANGUAGES CXX)

set(FETCHCONTENT_BASE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/.deps" CACHE PATH
    "FetchContent downloads (kept outside build/ for IDE include paths)")

include(FetchContent)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(USE_TLS OFF CACHE BOOL "Disable TLS for local ws:// server" FORCE)
set(USE_ZLIB OFF CACHE BOOL "Disable zlib to avoid extra dependency on MinGW" FORCE)
set(IXWEBSOCKET_INSTALL OFF CACHE BOOL "Skip installing ixwebsocket" FORCE)

FetchContent_Declare(
    ixwebsocket
    GIT_REPOSITORY https://github.com/machinezone/IXWebSocket.git
    GIT_TAG v11.4.5
)
FetchContent_MakeAvailable(ixwebsocket)

# Mirror ixwebsocket headers outside build/ so clangd/IntelliSense keep working
# after build/ is cleaned (FetchContent lives in the build tree by default).
set(IXWEBSOCKET_IDE_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/.deps/ixwebsocket-src")
if(EXISTS "${ixwebsocket_SOURCE_DIR}")
    file(COPY "${ixwebsocket_SOURCE_DIR}/" DESTINATION "${IXWEBSOCKET_IDE_INCLUDE_DIR}")
endif()

find_package(Threads REQUIRED)

set(OPENCV_DIR "C:/Users/saris/OneDrive/Documents/bootcamp/CTD26/cpp/OpenCV_451")
set(OPENCV_INCLUDE_DIR "${OPENCV_DIR}/include")
set(OPENCV_LIB_DIR "${OPENCV_DIR}/bin")

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src)

set(KONG_FU_CHESS_SOURCES
    src/model/board.cpp
    src/rules/movement/movement_helpers.cpp
    src/rules/movement/piece_movements.cpp
    src/rules/piece_rules.cpp
    src/rules/rule_engine.cpp
    src/realtime/real_time_arbiter.cpp
    src/engine/game_engine.cpp
    src/engine/move_log.cpp
    src/events/event_bus.cpp
    src/input/board_mapper.cpp
    src/input/controller.cpp
    src/io/board_parser.cpp
    src/io/board_printer.cpp
    src/io/algebraic.cpp
    src/io/board_setup.cpp
    src/texttests/script_parser.cpp
    src/texttests/script_runner.cpp
    src/session/session_manager.cpp
    src/session/shell_login.cpp
    src/network/ws_protocol.cpp
    src/network/command_queue.cpp
    src/network/network_input.cpp
)

add_executable(kong_fu_chess
    src/main.cpp
    ${KONG_FU_CHESS_SOURCES}
    src/network/ws_server.cpp
    src/network/local_ws_client.cpp
    src/view/img.cpp
    src/view/piece_sprite.cpp
    src/view/render_helpers.cpp
    src/view/renderer.cpp
)

target_include_directories(kong_fu_chess PRIVATE
    ${OPENCV_INCLUDE_DIR}
    ${ixwebsocket_SOURCE_DIR}
)
target_link_directories(kong_fu_chess PRIVATE ${OPENCV_LIB_DIR})

if(MSVC)
    target_link_libraries(kong_fu_chess PRIVATE
        $<$<CONFIG:Debug>:${OPENCV_LIB_DIR}/opencv_world451d.lib>
        $<$<CONFIG:Release>:${OPENCV_LIB_DIR}/opencv_world451.lib>
        $<$<CONFIG:RelWithDebInfo>:${OPENCV_LIB_DIR}/opencv_world451.lib>
        $<$<CONFIG:MinSizeRel>:${OPENCV_LIB_DIR}/opencv_world451.lib>
    )
else()
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        set(OPENCV_LIB_FILE "${OPENCV_LIB_DIR}/opencv_world451d.lib")
        set(OPENCV_DLL_FILE "${OPENCV_LIB_DIR}/opencv_world451d.dll")
    else()
        set(OPENCV_LIB_FILE "${OPENCV_LIB_DIR}/opencv_world451.lib")
        set(OPENCV_DLL_FILE "${OPENCV_LIB_DIR}/opencv_world451.dll")
    endif()
    target_link_libraries(kong_fu_chess PRIVATE "${OPENCV_LIB_FILE}")
endif()

target_link_libraries(kong_fu_chess PRIVATE ixwebsocket::ixwebsocket Threads::Threads)

if(WIN32)
    target_link_libraries(kong_fu_chess PRIVATE ws2_32 wsock32 shlwapi)

    if(MSVC)
        add_custom_command(TARGET kong_fu_chess POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<$<CONFIG:Debug>:${OPENCV_LIB_DIR}/opencv_world451d.dll>
            $<$<CONFIG:Release>:${OPENCV_LIB_DIR}/opencv_world451.dll>
            $<$<CONFIG:RelWithDebInfo>:${OPENCV_LIB_DIR}/opencv_world451.dll>
            $<$<CONFIG:MinSizeRel>:${OPENCV_LIB_DIR}/opencv_world451.dll>
            $<TARGET_FILE_DIR:kong_fu_chess>)
    else()
        add_custom_command(TARGET kong_fu_chess POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${OPENCV_DLL_FILE}"
            $<TARGET_FILE_DIR:kong_fu_chess>)
    endif()
endif()

add_executable(board_tests
    src/tests.cpp
    ${KONG_FU_CHESS_SOURCES}
)

if(MSVC)
    target_compile_options(kong_fu_chess PRIVATE /W4)
    target_compile_options(board_tests PRIVATE /W4)
else()
    target_compile_options(kong_fu_chess PRIVATE -Wall -Wextra -Wpedantic)
    target_compile_options(board_tests PRIVATE -Wall -Wextra -Wpedantic)
endif()
