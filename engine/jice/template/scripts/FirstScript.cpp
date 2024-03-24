#include "Engine/Script.h"


class FirstScript : public ScriptInterface {
public:

    explicit FirstScript(Engine* e, GameObject* o) : ScriptInterface(e, o) {
        // runs for each object with this script attatched
        // on engine startup / when scene is created

    }


    void Setup() override {
        // runs for each object with this script attatched
        // on scene setup / when scene is loaded

    }

    void Update() override {
        // runs for each object with this script attatched
        // on each frame

    }

    std::vector<std::string> getDependencies() override {
        // return a list of component names that this script depends on
        // most scripts will depend on Transform as almost every object has a transform, and it is the default
        // return an empty list if the script does not depend on any components

        return {Transform::COMPONENT_NAME};
    }
};