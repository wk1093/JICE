#pragma once
#include <string>
#include <cstdint>
#include <vector>

namespace jice {
    class Asset {
    private:
        std::vector<uint8_t> data; // we don't want this to be modifiable from outside

    public:

        Asset();

        explicit Asset(std::vector<uint8_t> data);

        [[nodiscard]] std::vector<uint8_t> getData() const;

        [[nodiscard]] size_t getSize() const;
    };

    Asset CompiledAsset(const uint8_t *data, size_t length);

    Asset CopiedAsset(const std::string &path);

}