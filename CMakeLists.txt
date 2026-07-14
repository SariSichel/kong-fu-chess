cmake_minimum_required(VERSION 3.16)
project(kong_fu_chess LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(OPENCV_DIR "C:/Users/saris/OneDrive/Documents/bootcamp/CTD26/cpp/OpenCV_451")
set(OPENCV_INCLUDE_DIR "${OPENCV_DIR}/include")
set(OPENCV_LIB_DIR "${OPENCV_DIR}/bin")

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
    src/io/board_setup.cpp
    src/texttests/script_parser.cpp
    src/texttests/script_runner.cpp
)

add_executable(kong_fu_chess
    src/main.cpp
    ${KONG_FU_CHESS_SOURCES}
    src/view/img.cpp
    src/view/renderer.cpp
)

target_include_directories(kong_fu_chess PRIVATE ${OPENCV_INCLUDE_DIR})
target_link_directories(kong_fu_chess PRIVATE ${OPENCV_LIB_DIR})

if(MSVC)
    target_link_libraries(kong_fu_chess
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
    target_link_libraries(kong_fu_chess "${OPENCV_LIB_FILE}")
endif()

if(WIN32)
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
