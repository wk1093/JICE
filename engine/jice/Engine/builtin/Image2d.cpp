#include <iostream>
#include "Engine/internal/RenderTask.h"
#include "Image2d.h"
#include "Transform.h"
#include "Engine/internal/Engine.h"

namespace jice {

    const std::string Image2d::COMPONENT_NAME = "image2d";

    const float image_vertices[] = {
            -1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
            1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
            1.0f, 1.0f, 0.0f, 1.0f, 1.0f,
            -1.0f, 1.0f, 0.0f, 0.0f, 1.0f
    };
    const unsigned int image_indices[] = {
            0, 1, 2,
            2, 3, 0
    };

    Image2d::Image2d(AttributeData data, bool create_buffer) : AttributeInterface(data) {
        image = "";
        if (data.find("image") != data.end()) {
            if (data["image"].type == AttrDataType::String) {
                image = data["image"].s;
            } else {
                std::cout << "Invalid data type for image" << std::endl;
            }
        } else {
            std::cout << "Image not found" << std::endl;
        }
        if (create_buffer) {

            unsigned int vao = eogllGenVertexArray();
            unsigned int vbo = eogllGenBuffer(vao, GL_ARRAY_BUFFER, sizeof(image_vertices), image_vertices,
                                              GL_STATIC_DRAW);
            unsigned int ebo = eogllGenBuffer(vao, GL_ELEMENT_ARRAY_BUFFER, sizeof(image_indices), image_indices,
                                              GL_STATIC_DRAW);
            EogllAttribBuilder builder = eogllCreateAttribBuilder();
            eogllAddAttribute(&builder, GL_FLOAT, 3);
            eogllAddAttribute(&builder, GL_FLOAT, 2);
            eogllBuildAttributes(&builder, vao);
            this->b_obj = eogllCreateBufferObject(vao, vbo, ebo, sizeof(image_indices), GL_UNSIGNED_INT);
        }
    }

    void Image2d::Update(Engine *e, GameObject *obj) {
        // create render task
        std::string info = image;
        // if contains a Transform component
        if (obj->hasComponent(Transform)) {
            auto *t = (Transform *) obj->getComponent(Transform);
            RenderTask task;
            task.texture = image;
            task.shader = "default_3f2f_pt";
            task.obj = &this->b_obj;
            task.mode = GL_TRIANGLES;
            task.simple = false;
            task.model = t->toModel();
            e->addRenderTask(task);
            // todo: add transform
        } else {
            std::cout << "No transform component found" << std::endl;
        }
    }

    std::vector<std::string> Image2d::getDependencies() {
        return {Transform::COMPONENT_NAME};
    }

}