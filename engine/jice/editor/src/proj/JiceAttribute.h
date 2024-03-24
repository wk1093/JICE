#pragma once

#include <string>
#include <nlohmann/json.hpp>
#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui.h>
#include <imgui_internal.h>
#include <imgui_stdlib.h>
#include "im_panel.h"
#include "Engine/builtin/Transform.h"
#include "Engine/builtin/Image2d.h"

using jice::AttributeData;
using jice::AttributeInterface;
using jice::Transform;
using jice::Image2d;
using jice::AttrData;
using jice::AttrDataType;

AttributeData toAttrData(nlohmann::json js) {
    AttributeData d = AttributeData();
    for (auto& [key, value] : js.items()) {
        if (value.is_number()) {
            if (value.is_number_integer()) {
                d[key] = AttrData(value.get<int>());
            } else {
                d[key] = AttrData(value.get<float>());
            }
        } else if (value.is_string()) {
            d[key] = AttrData(value.get<std::string>());
        } else if (value.is_array()) {
            if (value.size() == 0) {
                d[key] = AttrData();
            } else if (value[0].is_number()) {
                if (value[0].is_number_integer()) {
                    std::vector<int> vec;
                    for (auto& v : value) {
                        vec.push_back(v.get<int>());
                    }
                    d[key] = AttrData(vec);
                } else {
                    std::vector<float> vec;
                    for (auto& v : value) {
                        vec.push_back(v.get<float>());
                    }
                    d[key] = AttrData(vec);
                }
            } else {
                std::cout << "Unsupported array type" << std::endl;
            }
        } else {
            std::cout << "Unsupported type" << std::endl;
        }
    }
    return d;
}

nlohmann::json attrToJson(AttributeData d) {
    nlohmann::json j = nlohmann::json::object();
    for (auto& [key, value] : d) {
        if (value.type == AttrDataType::VecF) {
            nlohmann::json vec;
            for (auto& v : value.vecF) {
                vec.push_back(v);
            }
            j[key] = vec;
        } else if (value.type == AttrDataType::VecI) {
            nlohmann::json vec;
            for (auto& v : value.vecI) {
                vec.push_back(v);
            }
            j[key] = vec;
        } else if (value.type == AttrDataType::Float) {
            j[key] = value.f;
        } else if (value.type == AttrDataType::Int) {
            j[key] = value.i;
        } else if (value.type == AttrDataType::String) {
            j[key] = value.s;
        }
    }
    return j;
}

struct JiceAttribute {
    bool m_builtin;

    // if m_builtin false:
    std::string m_script;
    AttributeData m_data;

    // if m_builtin true:
    AttributeInterface* m_builtin_data;
    std::string m_id;

    JiceAttribute(const nlohmann::json& j) {
        if (!j.contains("type")) {
            throw std::runtime_error("Failed to find type");
        }
        if (j["type"] == "builtin") {
            m_builtin = true;
            if (!j.contains("id")) {
                throw std::runtime_error("Failed to find id");
            }
            m_id = j["id"];
            nlohmann::json data;
            if (j.contains("data")) {
                data = j["data"];
            }
            if (m_id == Transform::COMPONENT_NAME) {
                m_builtin_data = new Transform(toAttrData(data));
            } else if (m_id == Image2d::COMPONENT_NAME) {
                m_builtin_data = new Image2d(toAttrData(data), false);
            } else {
                throw std::runtime_error("Unknown builtin type");
            }
        } else if (j["type"] == "script") {
            m_builtin = false;
            if (!j.contains("location")) {
                throw std::runtime_error("Failed to find script");
            }
            m_script = j["location"];
            if (j.contains("data")) {
                m_data = toAttrData(j["data"]);
            }
        } else {
            throw std::runtime_error("Unknown type");
        }
    }

    static JiceAttribute Builtin(const std::string& id) {
        nlohmann::json j;
        j["type"] = "builtin";
        j["id"] = id;
        j["data"] = nlohmann::json();
        return JiceAttribute(j);
    }

    static JiceAttribute Script(const std::string& location) {
        nlohmann::json j;
        j["type"] = "script";
        j["location"] = location;
        j["data"] = nlohmann::json();
        return JiceAttribute(j);
    }

    bool operator==(const JiceAttribute& rhs) const {
        return m_builtin == rhs.m_builtin &&
               m_script == rhs.m_script &&
               m_data == rhs.m_data &&
               m_builtin_data == rhs.m_builtin_data;
    }

    nlohmann::json toJson() {
        nlohmann::json j;
        if (m_builtin) {
            j["type"] = "builtin";
            j["id"] = m_id;
            j["data"] = attrToJson(m_builtin_data->data);
        } else {
            j["type"] = "script";
            j["location"] = m_script;
            j["data"] = attrToJson(m_data);
        }
        return j;
    }
};