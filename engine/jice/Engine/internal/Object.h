#pragma once
#include <vector>
#include <string>
#include <unordered_map>

#include "Scripting.h"

namespace jice {

    enum AttrDataType {
        None,
        VecF,
        VecI,
        Float,
        Int,
        String
    };

    class AttrData {
    public:
        AttrDataType type;
        std::vector<float> vecF;
        std::vector<int> vecI;
        float f;
        int i;
        std::string s;

        explicit AttrData(std::vector<float> data);

        explicit AttrData(std::vector<int> data);

        explicit AttrData(float data);

        explicit AttrData(int data);

        explicit AttrData(std::string data);

        AttrData();

        ~AttrData();

        // operator=

        AttrData &operator=(const AttrData &other);

        // operator==
        bool operator==(const AttrData &other) const;
    };


    typedef std::unordered_map<std::string, AttrData> AttributeData;

    class GameObject;

    class AttributeInterface {
    public:
        AttributeData data;

        AttributeInterface(AttributeData data) : data(data) {}

        virtual void Update(Engine *e, GameObject *obj) = 0;

        virtual std::vector<std::string> getDependencies() = 0;
    };

    AttributeInterface *createBuiltinAttr(Engine *e, const std::string &id, const AttributeData &data);

    class Attribute {
    public:
        bool isScript;
        // script stuff
        ScriptInterface *script{};
        std::string scriptName;
        AttributeData data;

        // builtin stuff
        AttributeInterface *builtin;
        std::string id;

        Attribute(Engine *e, const std::string &id, const AttributeData &data);

        Attribute(ScriptInterface *scr, AttributeData data, const std::string &scr_name);

    };

    class ObjectInterface {
    public:
        std::vector<GameObject *> children;

        void addObject(GameObject *obj);

        void removeObject(GameObject *obj);

        GameObject *getObject(std::string name);

    };

    class GameObject : public ObjectInterface {
    public:
        std::string name;
        std::vector<Attribute *> attributes;

        explicit GameObject(std::string name);

        void addAttribute(Attribute *attr);

        template<typename T>
        T *getComponentFromName(const std::string &comp_name) {
            for (auto attr: attributes) {
                if (!attr->isScript) {
                    if (attr->id == comp_name) {
                        if (auto *casted = dynamic_cast<T *>(attr->builtin)) {
                            return casted;
                        } else {
                            printf("Cast fail\n");
                            return nullptr;
                        }
                    }
                } else {
                    if (attr->scriptName == comp_name) {
                        if (auto *casted = dynamic_cast<T *>(attr->script)) {
                            return casted;
                        } else {
                            printf("Cast fail\n");
                            return nullptr;
                        }

                    }
                }
            }
            return nullptr;
        }

        bool hasComponentFromName(const std::string &comp_name) {
            for (auto attr: attributes) {
                if (!attr->isScript) {
                    if (attr->id == comp_name) {
                        return true;
                    }
                } else {
                    if (attr->scriptName == comp_name) {
                        return true;
                    }
                }
            }
            return false;
        }
    };


// Instead of doing this:
// transform = obj->getComponentFromName<Transform>(Transform::COMPONENT_NAME);
// we want to do this:
// transform = obj->getComponent(Transform);
// using macros we can do this:
#define getComponent(type) getComponentFromName<type>(type::COMPONENT_NAME)
#define hasComponent(type) hasComponentFromName(type::COMPONENT_NAME)

}