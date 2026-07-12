cmake_minimum_required(VERSION 3.16)
project(kong_fu_chess LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src)

set(KONG_FU_CHESS_SOURCES
    src/model/board.cpp
    src/rules/piece_rules.cpp
    src/rules/rule_engine.cpp
    src/realtime/real_time_arbiter.cpp
    src/engine/game_engine.cpp
    src/input/board_mapper.cpp
    src/input/controller.cpp
    src/io/board_parser.cpp
    src/io/board_printer.cpp
    src/texttests/script_parser.cpp
    src/texttests/script_runner.cpp
)

add_executable(kong_fu_chess
    src/main.cpp
    ${KONG_FU_CHESS_SOURCES}
)

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
