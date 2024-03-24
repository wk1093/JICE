#include "Scripting.h"

#include <iostream>

namespace jice {

    void ScriptInterface::Setup() {
    }

    void ScriptInterface::Update() {
    }

    ScriptInterface::ScriptInterface(Engine *e, GameObject *o) {
        engine = e;
        obj = o;
    }

    std::vector<std::string> ScriptInterface::getDependencies() {
        return {};
    }

}