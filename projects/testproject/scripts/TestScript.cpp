#include "Engine/Script.h"
#include <iostream>


class TestScript : public ScriptInterface {
public:
    Transform* transform;

    explicit TestScript(Engine* e, GameObject* o) : ScriptInterface(e, o) {

    }


    void Setup() override {
        transform = obj->getComponent(Transform);
    }

    void Update() override {
        transform->position.x += 0.001f;
    }

    std::vector<std::string> getDependencies() override {
        return {Transform::COMPONENT_NAME};
    }
};