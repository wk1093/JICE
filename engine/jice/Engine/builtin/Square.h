#pragma once

#include <eogll.h>
#include "Engine/internal/Object.h"

namespace jice {
    class Square : public AttributeInterface {
    public:
        static const std::string COMPONENT_NAME;
        EogllBufferObject b_obj{};
        std::string shader;

        explicit Square(AttributeData data, bool create_buffer = true, const std::string& shader="default_3f_p");

        void Update(Engine *e, GameObject *obj) override;

        std::vector<std::string> getDependencies() override;
    };
}