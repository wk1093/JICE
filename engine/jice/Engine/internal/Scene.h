#pragma once
#include "Object.h"

namespace jice {

    class Engine;

    enum SceneMode {
        Dimension2D,
        Dimension3D
    };

    class Scene : public ObjectInterface {
    public:
        Engine *engine;
        std::string name;
        SceneMode mode;

        Scene(Engine *e);

        virtual void Setup();

        virtual void Update();
    };

}