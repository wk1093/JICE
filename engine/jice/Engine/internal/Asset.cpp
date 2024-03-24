#include "Asset.h"

#include <fstream>
#include <iostream>

namespace jice {

    Asset::Asset() = default;

    Asset::Asset(std::vector<uint8_t> data) {
        this->data = std::move(data);
    }

    std::vector<uint8_t> Asset::getData() const {
        return data;
    }

    size_t Asset::getSize() const {
        return data.size();
    }

    Asset CompiledAsset(const uint8_t *data, size_t length) {
        return Asset(std::vector<uint8_t>(data, data + length));
    }

    Asset CopiedAsset(const std::string &path) {
        std::ifstream file("assets/" + path, std::ios::binary);
        if (!file.is_open()) {
            std::cerr << "Failed to open file: " << path << std::endl;
        }
        std::vector<uint8_t> data((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
        return Asset(data);
    }

}