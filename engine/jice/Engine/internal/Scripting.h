#pragma once

#include <vector>
#include <string>

namespace jice {

    class GameObject;

    class Engine;

    class ScriptInterface {
    public:
        ScriptInterface(Engine *e, GameObject *o);

        Engine *engine;
        GameObject *obj;

        virtual void Setup();

        virtual void Update();

        virtual std::vector<std::string> getDependencies();
    };

}