cmake_minimum_required(VERSION 3.6)

include(FetchContent)

# C++ 17
set(CMAKE_CXX_STANDARD 17)

# get imgui docking branch
FetchContent_Declare(
        imgui
        GIT_REPOSITORY https://github.com/ocornut/imgui.git
        GIT_TAG        docking
)
# imgui doesn't use CMake, so we need to create a target for it
FetchContent_GetProperties(imgui)
if(NOT imgui_POPULATED)
    FetchContent_Populate(imgui)
    add_library(imgui STATIC
            ${imgui_SOURCE_DIR}/imgui.cpp
            ${imgui_SOURCE_DIR}/imgui_demo.cpp
            ${imgui_SOURCE_DIR}/imgui_draw.cpp
            ${imgui_SOURCE_DIR}/imgui_widgets.cpp
            ${imgui_SOURCE_DIR}/imgui_tables.cpp
            ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
            ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
            ${imgui_SOURCE_DIR}/misc/cpp/imgui_stdlib.cpp
    )
    target_include_directories(imgui PUBLIC ${imgui_SOURCE_DIR})
    target_include_directories(imgui PUBLIC ${imgui_SOURCE_DIR}/backends)
    target_include_directories(imgui PUBLIC ${imgui_SOURCE_DIR}/misc/cpp)
    target_link_libraries(imgui PUBLIC glad glfw)
endif()

# json
FetchContent_Declare(
        nlohmann_json
        GIT_REPOSITORY https://github.com/nlohmann/json.git
        GIT_TAG        master
)
set(JSON_BuildTests OFF CACHE INTERNAL "")
FetchContent_MakeAvailable(nlohmann_json)


add_executable(JiceEditor src/main.cpp
        src/editor_ui.h
        src/editor_ui.cpp
        src/menus/Menu.h
        src/proj/JiceProject.h
    src/proj/JiceScene.h
    src/proj/JiceObject.h
    src/proj/JiceAttribute.h
        src/proj/json_loadlib.h
        src/proj/JiceScene.h
        src/proj/JiceObject.h
        src/proj/JiceAttribute.h
        src/proj/json_loadlib.h
        src/im_panel.cpp
        src/im_panel.h
)

# Link the libraries
target_link_libraries(JiceEditor Engine imgui nlohmann_json)
target_include_directories(JiceEditor PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)