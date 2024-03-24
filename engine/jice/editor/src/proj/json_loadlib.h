#pragma once

#include <string>
#include <nlohmann/json.hpp>

bool verify_json(nlohmann::json j, int data_id, int engine_version) {
    if (j.contains("engine_version")) {
        if (j["engine_version"] != engine_version) {
            return false;
        }
    } else {
        return false;
    }
    if (j.contains("data_id")) {
        if (j["data_id"] != data_id) {
            return false;
        }
    } else {
        return false;
    }
    return true;
}

template<typename T>
T try_get(nlohmann::json j, std::string key, T def) {
    if (j.contains(key)) {
        return j[key];
    }
    return def;
}