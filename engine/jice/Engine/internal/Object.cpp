#include "Object.h"

#include <iostream>

namespace jice {

    AttrData::AttrData(std::vector<float> data) {
        type = VecF;
        vecF = data;
    }

    AttrData::AttrData(std::vector<int> data) {
        type = VecI;
        vecI = data;
    }

    AttrData::AttrData(float data) {
        type = Float;
        f = data;
    }

    AttrData::AttrData(int data) {
        type = Int;
        i = data;
    }

    AttrData::AttrData(std::string data) {
        type = String;
        s = data;
    }

    AttrData::AttrData() {
        type = None;
    }

    AttrData::~AttrData() {
        if (type == VecF) {
            vecF.clear();
        } else if (type == VecI) {
            vecI.clear();
        }
    }

    AttrData &AttrData::operator=(const AttrData &other) {
        if (this != &other) {
            if (type == VecF) {
                vecF.clear();
            } else if (type == VecI) {
                vecI.clear();
            }
            type = other.type;
            if (type == VecF) {
                vecF = other.vecF;
            } else if (type == VecI) {
                vecI = other.vecI;
            } else if (type == Float) {
                f = other.f;
            } else if (type == Int) {
                i = other.i;
            } else if (type == String) {
                s = other.s;
            }
        }
        return *this;
    }

    bool AttrData::operator==(const AttrData &other) const {
        if (type != other.type) {
            return false;
        }
        if (type == VecF) {
            return vecF == other.vecF;
        } else if (type == VecI) {
            return vecI == other.vecI;
        } else if (type == Float) {
            return f == other.f;
        } else if (type == Int) {
            return i == other.i;
        } else if (type == String) {
            return s == other.s;
        } else {
            std::cout << "Unsupported type" << std::endl;
            return true;
        }
    }

    Attribute::Attribute(Engine *e, const std::string &id, const AttributeData &data) {
        isScript = false;
        this->id = id;
        script = nullptr;
        builtin = createBuiltinAttr(e, id, data);
    }

    Attribute::Attribute(ScriptInterface *scr, AttributeData data, const std::string &scr_name) {
        isScript = true;
        script = scr;
        scriptName = scr_name;
        id = "script";
        this->data = std::move(data);
        builtin = nullptr;
    }

    void ObjectInterface::addObject(GameObject *obj) {
        children.push_back(obj);
    }

    void ObjectInterface::removeObject(GameObject *obj) {
        for (int i = 0; i < children.size(); i++) {
            if (children[i] == obj) {
                children.erase(children.begin() + i);
                break;
            }
        }
    }

    GameObject *ObjectInterface::getObject(std::string name) {
        for (int i = 0; i < children.size(); i++) {
            if (children[i]->name == name) {
                return children[i];
            }
        }
        return nullptr;
    }

    GameObject::GameObject(std::string name) {
        this->name = std::move(name);
    }

    void GameObject::addAttribute(Attribute *attr) {
        attributes.push_back(attr);
    }

}
