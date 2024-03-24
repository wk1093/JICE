#pragma once

#include <string>
#include <vector>
#include <filesystem>
#include "JiceObject.h"
#include "json_loadlib.h"

struct JiceScene {
    std::string m_name;
    std::string m_path;
    bool m_3d;
    std::vector<JiceObject*> m_objects;

    explicit JiceScene(const std::filesystem::path& path_to_scene_file) {
        if (!std::filesystem::exists(path_to_scene_file)) {
            std::cout << "Failed to find scene file" << std::endl;
            return;
        }
        std::ifstream scene_file(path_to_scene_file);
        if (!scene_file.is_open()) {
            std::cout << "Failed to open scene file" << std::endl;
            return;
        }

        m_path = path_to_scene_file.string();

        nlohmann::json j;
        scene_file >> j;
        scene_file.close();

        if (!verify_json(j, 1, 100)) {
            std::cout << "Failed to verify json" << std::endl;
            return;
        }

        if (!j.contains("data")) {
            std::cout << "Failed to find data" << std::endl;
            return;
        }
        nlohmann::json data = j["data"];
        try {
            m_name = data["name"];
            m_3d = try_get(data, "3d", false);
        } catch (nlohmann::json::exception& e) {
            std::cout << "Failed to parse scene file: " << e.what() << std::endl;
            return;
        }
        if (data.contains("content")) {
            for (auto& obj : data["content"]) {
                m_objects.push_back(new JiceObject(obj));
            }
        }

    }

    bool save() {
        nlohmann::json j;
        j["engine_version"] = 100;
        j["data_id"] = 1;
        j["data"] = {
            {"name", m_name},
            {"3d", m_3d},
            {"content", nlohmann::json::array()}
        };
        for (auto& obj : m_objects) {
            j["data"]["content"].push_back(obj->toJson());
        }
        // backup
        if (std::filesystem::exists(m_path)) {
            if (std::filesystem::exists(m_path + ".bak")) {
                std::filesystem::remove(m_path + ".bak");
            }
            std::filesystem::copy(m_path, m_path + ".bak", std::filesystem::copy_options::overwrite_existing);
        }
        std::ofstream scene_file(m_path);
        if (!scene_file.is_open()) {
            std::cout << "Failed to open scene file for writing" << std::endl;
            return false;
        }
        scene_file << j.dump(4);
        scene_file.close();
        return true;
    }

};

bool removeJiceObjectFromScene(JiceScene& scene, const JiceObject* obj) {
    for (auto& child : scene.m_objects) {
        if (child->m_id == obj->m_id) {
            scene.m_objects.erase(std::remove(scene.m_objects.begin(), scene.m_objects.end(), child), scene.m_objects.end());
            return true;
        }
    }
    for (auto& child : scene.m_objects) {
        if (removeJiceObjectFromParent(child, obj)) {
            return true;
        }
    }
    return false;
}

bool isChildOfScene(const JiceScene& scene, const JiceObject* obj) {
    for (auto& child : scene.m_objects) {
        if (child->m_id == obj->m_id) {
            return true;
        }
    }
    for (auto& child : scene.m_objects) {
        if (isChildOf(child, obj)) {
            return true;
        }
    }
    return false;
}