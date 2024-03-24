#pragma once

#include <eogll.h>
#include "Engine/internal/Object.h"

namespace jice {

    class Image2d : public AttributeInterface {
    public:
        static const std::string COMPONENT_NAME;
        std::string image;
        EogllBufferObject b_obj{};

        explicit Image2d(AttributeData data, bool create_buffer = true);

        void Update(Engine *e, GameObject *obj) override;

        std::vector<std::string> getDependencies() override;
    };

}