#include "Square.h"

#include <iostream>
#include "Engine/internal/RenderTask.h"
#include "Transform.h"
#include "Engine/internal/Engine.h"

namespace jice {
    const std::string Square::COMPONENT_NAME = "square";

    const float square_vertices[] = {
            -1.0f, -1.0f, 0.0f,
            1.0f, -1.0f, 0.0f,
            1.0f, 1.0f, 0.0f,
            -1.0f, 1.0f, 0.0f
    };
    const unsigned int square_indices[] = {
            0, 1, 2,
            2, 3, 0
    };

    Square::Square(AttributeData data, bool create_buffer, const std::string& shader) : AttributeInterface(data) {
        this->shader = shader;
        if (create_buffer) {
            unsigned int vao = eogllGenVertexArray();
            unsigned int vbo = eogllGenBuffer(vao, GL_ARRAY_BUFFER, sizeof(square_vertices), square_vertices,
                                              GL_STATIC_DRAW);
            unsigned int ebo = eogllGenBuffer(vao, GL_ELEMENT_ARRAY_BUFFER, sizeof(square_indices), square_indices,
                                              GL_STATIC_DRAW);
            EogllAttribBuilder builder = eogllCreateAttribBuilder();
            eogllAddAttribute(&builder, GL_FLOAT, 3);
            eogllBuildAttributes(&builder, vao);
            this->b_obj = eogllCreateBufferObject(vao, vbo, ebo, sizeof(square_indices), GL_UNSIGNED_INT);
        }
    }

    void Square::Update(Engine *e, GameObject *obj) {
        if (obj->hasComponent(Transform)) {
            auto *t = (Transform *) obj->getComponent(Transform);
            RenderTask task;
            task.shader = shader;
            task.obj = &this->b_obj;
            task.mode = GL_TRIANGLES;
            task.simple = false;
            task.model = t->toModel();
            e->addRenderTask(task);
        } else {
            std::cout << "No transform component found" << std::endl;
        }

    }

    std::vector<std::string> Square::getDependencies() {
        return {Transform::COMPONENT_NAME};
    }


}
