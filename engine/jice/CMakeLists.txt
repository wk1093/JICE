cmake_minimum_required(VERSION 3.6)
project(JICE) # Json into C++ Engine

# _CRT_SECURE_NO_WARNINGS
add_definitions(-D_CRT_SECURE_NO_WARNINGS)

option(JICE_BUILD_EDITOR "Build the editor" OFF)

include(FetchContent)


FetchContent_Declare(
        eogll
        GIT_REPOSITORY https://github.com/wk1093/eogll.git
        GIT_TAG        master
)
FetchContent_MakeAvailable(eogll)

FetchContent_Declare(
        Boxer
        GIT_REPOSITORY https://github.com/aaronmjacobs/Boxer.git
        GIT_TAG        master
)
FetchContent_MakeAvailable(Boxer)

add_library(Engine STATIC Engine/internal/Asset.cpp Engine/internal/Asset.h
        Engine/internal/Engine.cpp Engine/internal/Engine.h Engine/internal/Object.cpp
        Engine/internal/Object.h Engine/internal/Scene.cpp Engine/internal/Scene.h
        Engine/Script.h Engine/internal/Scripting.cpp Engine/internal/Scripting.h
        Engine/Version.h
        Engine/builtin/builtin.cpp
        Engine/builtin/Transform.h
        Engine/builtin/Transform.h
        Engine/builtin/Transform.cpp
        Engine/math/Vector.h
        Engine/builtin/Image2d.h
        Engine/builtin/Image2d.cpp
        Engine/builtin/builtin.h
        Engine/internal/RenderTask.h
        Engine/util/Popup.h
        Engine/util/Popup.cpp
)

target_link_libraries(Engine PUBLIC eogll Boxer)

target_include_directories(Engine PUBLIC .)

add_subdirectory(jicc)

if(JICE_BUILD_EDITOR)
    add_subdirectory(editor)
endif()

