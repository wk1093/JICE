#pragma once

#include <string>
#include <vector>
#include <iostream>
#include <filesystem>
#include <fstream>
#include "JiceScene.h"
#include "json_loadlib.h"


struct JiceProject {
    std::string m_path;
    std::string m_id;
    std::string m_name;
    std::string m_desc;
    std::string m_version;
    std::string m_author;
    bool m_splash;
    std::string m_splashAsset;
    std::string m_assetLoc;
    std::string m_scriptLoc;
    std::string m_sceneLoc;

    std::vector<JiceScene> m_scenes;

    explicit JiceProject(const std::string& path_to_proj_file) {
        std::filesystem::path proj_path(path_to_proj_file);
        auto path_to_project_dir = proj_path.parent_path();
        m_path = path_to_project_dir.string();
        if (!std::filesystem::exists(proj_path)) {
            std::cout << "Failed to find proj.json" << std::endl;
            return;
        }
        std::ifstream proj_file(proj_path);
        if (!proj_file.is_open()) {
            std::cout << "Failed to open proj.json" << std::endl;
            return;
        }

        nlohmann::json j;
        proj_file >> j;
        proj_file.close();

        if (!verify_json(j, 0, 100)) {
            std::cout << "Failed to verify json" << std::endl;
            return;
        }
        // instead of a bunch of if exists, then get, and all that, we can just use a big try catch
        if (!j.contains("data")) {
            std::cout << "Failed to find data" << std::endl;
            return;
        }
        nlohmann::json data = j["data"];
        try {
            m_id = data["id"]; // if this fails, it will throw an exception
            m_name = try_get(data, "name", m_id);
            m_desc = try_get(data, "description", std::string());
            m_version = try_get(data, "version", std::string("0.0.1"));
            m_author = try_get(data, "author", std::string("Unknown"));
            if (data.contains("splash_screen")) {
                m_splash = try_get(data["splash_screen"], "enabled", false);
                if (m_splash)
                    m_splashAsset = try_get(data["splash_screen"], "image", std::string());
            } else {
                m_splash = false;
            }
        } catch (nlohmann::json::exception& e) {
            std::cout << "Failed to parse project data" << std::endl;
            std::cout << e.what() << std::endl;
            return;
        }
        m_assetLoc = "assets";
        m_scriptLoc = "scripts";
        m_sceneLoc = "scenes";
        try {
            nlohmann::json content = data["content"];
            m_assetLoc = try_get(content, "asset_loc", m_assetLoc);
            m_scriptLoc = try_get(content, "script_loc", m_scriptLoc);
            m_sceneLoc = try_get(content, "scene_loc", m_sceneLoc);
        } catch (nlohmann::json::exception& e) {
            // do nothing
        }

        // for all json files in the scene location, load them as scenes
        for (const auto& entry : std::filesystem::recursive_directory_iterator(path_to_project_dir / m_sceneLoc)) {
            if (entry.path().extension() == ".json") {
                m_scenes.push_back(JiceScene(entry.path()));
            }
        }


    }

    bool save() {
        nlohmann::json j;
        j["engine_version"] = 100;
        j["data_id"] = 0;
        j["data"]["id"] = m_id;
        j["data"]["name"] = m_name;
        j["data"]["description"] = m_desc;
        j["data"]["version"] = m_version;
        j["data"]["author"] = m_author;
        j["data"]["splash_screen"]["enabled"] = m_splash;
        j["data"]["splash_screen"]["image"] = m_splashAsset;
        j["data"]["content"]["asset_path"] = m_assetLoc;
        j["data"]["content"]["script_path"] = m_scriptLoc;
        j["data"]["content"]["scene_path"] = m_sceneLoc;
        auto proj_path = std::filesystem::path(m_path) / "proj.json";
        // backup
        if (std::filesystem::exists(proj_path)) {
            if (std::filesystem::exists(proj_path.string() + ".bak")) {
                std::filesystem::remove(proj_path.string() + ".bak");
            }
            std::filesystem::copy(proj_path, proj_path.string() + ".bak", std::filesystem::copy_options::overwrite_existing);
        }
        std::ofstream proj_file(proj_path);
        if (!proj_file.is_open()) {
            std::cout << "Failed to open proj.json for writing" << std::endl;
            return false;
        }
        proj_file << j.dump(4);
        proj_file.close();
        // save all scenes
        bool success = true;
        for (auto& scene : m_scenes) {
            success &= scene.save();
        }
        return success;
    }
};