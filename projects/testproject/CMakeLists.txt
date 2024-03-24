cmake_minimum_required(VERSION 3.6)
project(game_build)

# if not target Engine exists
if(NOT TARGET Engine)
    # if flag JICE_DIR set, add_subdirectory to it
    if(DEFINED JICE_DIR)
        add_subdirectory(${JICE_DIR} engine)
    else()
        message(FATAL_ERROR "JICE_DIR not set, please set it to the engine directory")
    endif()
endif()

jice_compile(${CMAKE_CURRENT_SOURCE_DIR})