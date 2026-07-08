cmake_minimum_required(VERSION 3.16)
project(kong_fu_chess LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(KONG_FU_CHESS_SOURCES
    src/board.cpp
    src/board_serializer.cpp
    src/game_state.cpp
    src/command_processor.cpp
)

add_executable(kong_fu_chess
    src/main.cpp
    ${KONG_FU_CHESS_SOURCES}
)

add_executable(board_tests
    src/tests.cpp
    ${KONG_FU_CHESS_SOURCES}
)

target_include_directories(board_tests PRIVATE src)

if(MSVC)
    target_compile_options(kong_fu_chess PRIVATE /W4)
    target_compile_options(board_tests PRIVATE /W4)
else()
    target_compile_options(kong_fu_chess PRIVATE -Wall -Wextra -Wpedantic)
    target_compile_options(board_tests PRIVATE -Wall -Wextra -Wpedantic)
endif()
