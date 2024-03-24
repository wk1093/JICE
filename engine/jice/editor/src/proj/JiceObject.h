#pragma once

#include <string>
#include <vector>
#include "JiceAttribute.h"

struct JiceObject {
    std::string m_id;
    std::vector<JiceAttribute> m_attrs;
    std::vector<JiceObject*> m_children;

    explicit JiceObject(const nlohmann::json& j) {
        if (!j.contains("id")) {
            throw std::runtime_error("Failed to find id");
        }
        m_id = j["id"];
        if (j.contains("attributes")) {
            for (auto& attr : j["attributes"]) {
                m_attrs.push_back(JiceAttribute(attr));
            }
        }
        if (j.contains("children")) {
            for (auto& child : j["children"]) {
                m_children.push_back(new JiceObject(child));
            }
        }
    }

    explicit JiceObject(const std::string& id) : m_id(id) {}

    bool operator==(const JiceObject& rhs) const {
        return m_id == rhs.m_id &&
               m_attrs == rhs.m_attrs &&
               m_children == rhs.m_children;
    }

    bool hasAttribute(const std::string& id) const {
        for (auto& attr : m_attrs) {
            if (attr.m_id == id) {
                return true;
            }
            if (attr.m_script == id) {
                return true;
            }
        }
        return false;
    }

    nlohmann::json toJson() {
        nlohmann::json j;
        j["id"] = m_id;
        nlohmann::json attrs = nlohmann::json::array();
        for (auto& attr : m_attrs) {
            attrs.push_back(attr.toJson());
        }
        j["attributes"] = attrs;
        // declare as an array so if it is empty it will still be an array not null
        nlohmann::json children = nlohmann::json::array();
        for (auto& child : m_children) {
            children.push_back(child->toJson());
        }
        j["children"] = children;
        return j;
    }
};

bool removeJiceObjectFromParent(JiceObject* parent, const JiceObject* obj) {
    for (auto& child : parent->m_children) {
        if (child->m_id == obj->m_id) {
            parent->m_children.erase(std::remove(parent->m_children.begin(), parent->m_children.end(), child), parent->m_children.end());
            return true;
        }
    }
    for (auto& child : parent->m_children) {
        if (removeJiceObjectFromParent(child, obj)) {
            return true;
        }
    }
    return false;
}

bool isChildOf(const JiceObject* parent, const JiceObject* obj) {
    for (auto& child : parent->m_children) {
        if (child->m_id == obj->m_id) {
            return true;
        }
    }
    for (auto& child : parent->m_children) {
        if (isChildOf(child, obj)) {
            return true;
        }
    }
    return false;
}

bool isParentOf(const JiceObject* parent, const JiceObject* obj) {
    for (auto& child : obj->m_children) {
        if (child->m_id == parent->m_id) {
            return true;
        }
    }
    for (auto& child : obj->m_children) {
        if (isParentOf(parent, child)) {
            return true;
        }
    }
    return false;
}

void drawAttribute(JiceAttribute& attr, JiceObject* obj, int index) {
    if (attr.m_builtin) {
        if (ImGui::BeginGroupPanelButton(attr.m_id.c_str(), "Remove")) {
            std::cout << "removing " << attr.m_id << index << std::endl;
            obj->m_attrs.erase(obj->m_attrs.begin() + index);
            ImGui::EndGroupPanel();
            return;
        }

        if (attr.m_id == "transform") {
            ImGui::Text("Position");
            ImGui::InputFloat3("##position", ((Transform*)attr.m_builtin_data)->position.data);

            ImGui::Text("Rotation");
            ImGui::InputFloat3("##rotation", ((Transform*)attr.m_builtin_data)->rotation.data);

            ImGui::Text("Scale");
            ImGui::InputFloat3("##scale", ((Transform*)attr.m_builtin_data)->scale.data);
        } else if (attr.m_id == "image2d") {
            ImGui::Text("Image");
            ImGui::InputText("##image", &((Image2d*)attr.m_builtin_data)->image);
        }

        ImGui::EndGroupPanel();



    } else {
        if (ImGui::BeginGroupPanelButton(attr.m_script.c_str(), "Remove")) {
            std::cout << "removing " << attr.m_script << index << std::endl;
            obj->m_attrs.erase(obj->m_attrs.begin() + index);
            ImGui::EndGroupPanel();
            return;
        }

        // if attrdata is empty add some padding
        if (attr.m_data.empty()) {
            ImGui::Text("No data");
        } else {
            for (auto& [_key, value] : attr.m_data) {
                std::string key = "##" + _key;
                ImGui::Text("%s", _key.c_str());
                if (value.type == AttrDataType::Float) {
                    ImGui::InputFloat(key.c_str(), &value.f);
                } else if (value.type == AttrDataType::Int) {
                    ImGui::InputInt(key.c_str(), &value.i);
                } else if (value.type == AttrDataType::String) {
                    ImGui::InputText(key.c_str(), &value.s);
                } else if (value.type == AttrDataType::VecF) {
                    for (int i = 0; i < value.vecF.size(); i++) {
                        ImGui::InputFloat((key + std::to_string(i)).c_str(), &value.vecF[i]);
                    }
                } else if (value.type == AttrDataType::VecI) {
                    for (int i = 0; i < value.vecI.size(); i++) {
                        ImGui::InputInt((key + std::to_string(i)).c_str(), &value.vecI[i]);
                    }
                }
            }
        }

        ImGui::EndGroupPanel();
    }
}
