#pragma once

#include <string>
#include <eogll.h>

namespace jice {

    class RenderTask {
    public:
        std::string texture{};
        std::string shader = "default";
        EogllBufferObject *obj = nullptr;
        unsigned int mode = GL_TRIANGLES;
        bool simple = false;
        EogllModel model;
    };

}