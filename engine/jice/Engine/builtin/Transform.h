#pragma once

#include "Engine/internal/Object.h"
#include "Engine/math/Vector.h"
#include <eogll.h>

namespace jice {

    class Transform : public AttributeInterface {
    public:
        static const std::string COMPONENT_NAME;
        Vec3 position;
        Vec3 rotation;
        Vec3 scale;

        explicit Transform(AttributeData data);

        void Update(Engine *e, GameObject *obj) override;

        std::vector<std::string> getDependencies() override;

        friend std::ostream &operator<<(std::ostream &os, const Transform &transform);

        EogllModel toModel();
    };

}