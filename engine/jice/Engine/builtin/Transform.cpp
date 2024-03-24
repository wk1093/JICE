#include <iostream>
#include "Transform.h"

namespace jice {

    const std::string Transform::COMPONENT_NAME = "transform";

    Transform::Transform(AttributeData data) : AttributeInterface(data) {
        position = Vec3(0, 0, 0);
        rotation = Vec3(0, 0, 0);
        scale = Vec3(1, 1, 1);
        if (data.find("position") != data.end()) {
            if (data["position"].type == AttrDataType::VecF) {
                position = Vec3(data["position"].vecF[0], data["position"].vecF[1], data["position"].vecF[2]);
            } else if (data["position"].type == AttrDataType::VecI) {
                position = Vec3((float) data["position"].vecI[0], (float) data["position"].vecI[1],
                                (float) data["position"].vecI[2]);
            } else {
                std::cout << "Invalid data type for position" << std::endl;

            }
        }
        if (data.find("rotation") != data.end()) {
            if (data["rotation"].type == AttrDataType::VecF) {
                rotation = Vec3(data["rotation"].vecF[0], data["rotation"].vecF[1], data["rotation"].vecF[2]);
            } else if (data["rotation"].type == AttrDataType::VecI) {
                rotation = Vec3((float) data["rotation"].vecI[0], (float) data["rotation"].vecI[1],
                                (float) data["rotation"].vecI[2]);
            } else {
                std::cout << "Invalid data type for rotation" << std::endl;
            }
        }
        if (data.find("scale") != data.end()) {
            if (data["scale"].type == AttrDataType::VecF) {
                scale = Vec3(data["scale"].vecF[0], data["scale"].vecF[1], data["scale"].vecF[2]);
            } else if (data["scale"].type == AttrDataType::VecI) {
                scale = Vec3((float) data["scale"].vecI[0], (float) data["scale"].vecI[1],
                             (float) data["scale"].vecI[2]);
            } else {
                std::cout << "Invalid data type for scale" << std::endl;
            }
        }
    }

    void Transform::Update(Engine *e, GameObject *obj) {
        // empty for now
    }

    std::ostream &operator<<(std::ostream &os, const Transform &transform) {
        os << "Position: " << transform.position << " Rotation: " << transform.rotation << " Scale: "
           << transform.scale;
        return os;
    }

    std::vector<std::string> Transform::getDependencies() {
        return {};
    }

    EogllModel Transform::toModel() {
        EogllModel model;
        model.pos[0] = position.x;
        model.pos[1] = position.y;
        model.pos[2] = position.z;
        model.rot[0] = rotation.x;
        model.rot[1] = rotation.y;
        model.rot[2] = rotation.z;
        model.scale[0] = scale.x;
        model.scale[1] = scale.y;
        model.scale[2] = scale.z;
        return model;
    }

}