#include "builtin.h"
#include "Engine/internal/Engine.h"

namespace jice {
    AttributeInterface *createBuiltinAttr(Engine *e, const std::string &id, const AttributeData &data) {
        if (id == Transform::COMPONENT_NAME) {
            return new Transform(data);
        } else if (id == Image2d::COMPONENT_NAME) {
            return new Image2d(data);
        } else if (id == Square::COMPONENT_NAME) {
            return new Square(data);
        } else {
            printf("Error: Builtin attribute %s not found\n", id.c_str());
            return nullptr;
        }
    }

}