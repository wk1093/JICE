#pragma once
#ifndef BUILD_ENGINE_VERSION_H
#define BUILD_ENGINE_VERSION_H

// version 100

#define ENGINE_VERSION 100

#define CHECK_ENGINE_VERSION(x) static_assert(ENGINE_VERSION == (x), "Engine version mismatch");

namespace jice {
    class Engine;
    class GameObject;
    class ScriptInterface;
    class Scene;
}

#endif //BUILD_ENGINE_VERSION_H
