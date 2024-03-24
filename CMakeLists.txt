cmake_minimum_required(VERSION 3.6)
project(jice)

# change option JICE_BUILD_EDITOR
option(JICE_BUILD_EDITOR "Build JICE Editor" ON)

add_subdirectory(engine/jice)

add_subdirectory(projects/testproject)