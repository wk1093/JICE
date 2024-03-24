#include "Engine.h"
#include "Engine/util/Popup.h"
#include "Asset.h"

#include <iostream>

namespace jice {

    Engine::Engine(
            std::string id,
            std::string name,
            std::string description,
            std::string version,
            std::string author
    ) {
        if (eogllInit() != EOGLL_SUCCESS) {
            std::cerr << "JICE Failed to initialize EOGLL" << std::endl;
            return;
        }
        this->id = id;
        this->name = name;
        this->description = description;
        this->version = version;
        this->author = author;

        EogllWindowHints hints = eogllDefaultWindowHints();
        hints.resizable = false;
        hints.focused = false;
        hints.visible = false;
        ewindow = eogllCreateWindow(800, 600, name.c_str(), hints);
        // vsync
        glfwSwapInterval(1);
        eogllCenterWindow(ewindow);
        eogllEnableTransparency();

        if (ewindow == nullptr) {
            std::cerr << "Failed to create window" << std::endl;
            return;
        }
    }


    Engine::~Engine() {
        eogllTerminate();
    }

    void Engine::beginSplash(const std::string &assetLoc) {
        std::cout << "Starting splash..." << std::endl;
        std::cout << assetLoc << std::endl;
        isSplash = true;

        splashThread = std::thread(&Engine::keepSplashAlive, this, assetLoc);
    }

    void Engine::registerScript(const std::string &scr_name, ScriptDispatcher dispatcher) {
        scripts[scr_name] = dispatcher;
    }

    void Engine::addScene(const std::string &sc_name, Scene *scene) {
        scenes[sc_name] = scene;
    }

    void Engine::loadConfig(const std::string &path) {
        std::cout << "Loading config..." << std::endl;
        std::cout << path << std::endl;
    }

    void Engine::endSplash() {
        std::cout << "Ending splash..." << std::endl;
        isSplash = false;
        if (splashThread.joinable()) {
            splashThread.join();
        } else {
            std::cerr << "Splash thread not joinable" << std::endl;
        }
    }

    void Engine::update() {
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        currentScene->Update();
        for (const auto &task: renderTasks) {
            if (task.simple) {
                std::cout << "NOT SUPPORTED YET" << std::endl;
            } else {
                EogllShaderProgram *shader = getShader(task.shader);
                if (shader == nullptr) {
                    std::cerr << "Failed to get shader" << std::endl;
                    continue;
                }
                if (task.texture.empty()) {
                    std::cerr << "Texture not found" << std::endl;
                    continue;
                }
                EogllTexture *texture = getTexture(task.texture);
                if (texture == nullptr) {
                    std::cerr << "Failed to get texture" << std::endl;
                    ErrorPopupWindow("Error", "Could not load texture: " + task.texture);
                    continue;
                }
                eogllUseProgram(shader);
                eogllBindTexture(texture);
                eogllUpdateModelMatrix(&task.model, shader, "model");
                eogllDrawBufferObject(task.obj, task.mode);
            }

        }

        renderTasks.clear();
        eogllSwapBuffers(ewindow);
        eogllPollEvents(ewindow);
        if (eogllWindowShouldClose(ewindow)) {
            isRunning = false;
        }
    }

    void Engine::setup() {
        glfwShowWindow(ewindow->window);
        glfwFocusWindow(ewindow->window);
        // get first scene
        currentScene = scenes.begin()->second;
        currentScene->Setup();
        isRunning = true;
    }

    void Engine::run() {
        setup();
        while (isRunning) {
            update();
        }
        eogllDestroyWindow(ewindow);
        eogllTerminate();
    }

    void Engine::close() {
        isRunning = false;
    }

    ScriptInterface *Engine::dispatchScript(const std::string &scr_name, GameObject *obj) {
        return scripts[scr_name](this, obj);
    }

    void Engine::addRenderTask(const RenderTask &task) {
        renderTasks.push_back(task);
    }

    void Engine::addAsset(const std::string &asset_name, Asset asset) {
        assets[asset_name] = std::move(asset);
    }

    void Engine::keepSplashAlive(const std::string &assetLoc) {
        EogllWindowHints hints = eogllDefaultWindowHints();
        hints.resizable = false;
        hints.decorated = false;
        hints.focused = true;
        hints.floating = true;
        hints.transparent = true;
        EogllWindow *splashWindow = eogllCreateWindow(800, 600, "Splash", hints);
        eogllCenterWindow(splashWindow);
        eogllEnableTransparency();

        if (splashWindow == nullptr) {
            std::cerr << "Failed to create splash window" << std::endl;
            return;
        }
        float vertices[] = {
                -1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
                1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
                1.0f, 1.0f, 0.0f, 1.0f, 1.0f,
                -1.0f, 1.0f, 0.0f, 0.0f, 1.0f
        };
        unsigned int indices[] = {
                0, 1, 2,
                2, 3, 0
        };
        const char *vert = R"(
#version 330 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec2 aTexCoord;

out vec2 TexCoord;

void main() {
    gl_Position = vec4(aPos, 1.0);
    TexCoord = aTexCoord;
}
    )";
        const char *frag = R"(
#version 330 core
out vec4 FragColor;

in vec2 TexCoord;

uniform sampler2D texture1;

void main() {
    FragColor = texture(texture1, TexCoord);
}
    )";

        EogllShaderProgram *splashProgram = eogllLinkProgram(vert, frag);

        EogllAttribBuilder builder = eogllCreateAttribBuilder();
        eogllAddAttribute(&builder, GL_FLOAT, 3);
        eogllAddAttribute(&builder, GL_FLOAT, 2);

        unsigned int vao = eogllGenVertexArray();
        unsigned int vbo = eogllGenBuffer(vao, GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
        unsigned int ebo = eogllGenBuffer(vao, GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
        eogllBuildAttributes(&builder, vao);
        EogllBufferObject splashBuffer = eogllCreateBufferObject(vao, vbo, ebo, sizeof(indices), GL_UNSIGNED_INT);

        Asset a = getAsset(assetLoc);
        EogllTexture *splashTexture = eogllCreateTextureFromBuffer(a.getData().data(), a.getData().size());

        glfwFocusWindow(splashWindow->window);

        while (isSplash && !eogllWindowShouldClose(splashWindow)) {
            glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
            glClear(GL_COLOR_BUFFER_BIT);

            eogllUseProgram(splashProgram);
            eogllBindTexture(splashTexture);
            eogllDrawBufferObject(&splashBuffer, GL_TRIANGLES);

            eogllSwapBuffers(splashWindow);
            eogllPollEvents(splashWindow);
        }
        eogllDestroyWindow(splashWindow);
        eogllDeleteProgram(splashProgram);
        eogllDeleteTexture(splashTexture);
        eogllDeleteBufferObject(&splashBuffer);
    }

    EogllTexture *Engine::getTexture(const std::string &tex_name) {
        // if texture exists in map
        if (textures.find(tex_name) != textures.end()) {
            return textures[tex_name];
        }
        Asset a = getAsset(tex_name);
        std::cout << "Creating texture '" << tex_name << "'" << std::endl;
        EogllTexture *texture = eogllCreateTextureFromBuffer(a.getData().data(), a.getData().size());
        textures[tex_name] = texture;
        return texture;

    }

    EogllShaderProgram *Engine::getShader(const std::string &shader_name) {
        if (shaders.find(shader_name) != shaders.end()) {
            return shaders[shader_name];
        }
        // shader program test_3f2f_pt is a program that would be created by having the files:
        // test_3f2f_pt.(vert/vs) and test_3f2f_pt.(frag/fs)
        // in the assets folder
        // this asset can be any type of asset
        Asset vertAsset;
        if (assets.find(shader_name + ".vert") != assets.end()) {
            vertAsset = assets[shader_name + ".vert"];
        } else if (assets.find(shader_name + ".vs") != assets.end()) {
            vertAsset = assets[shader_name + ".vs"];
        } else {
            getAsset(shader_name + ".vert");
            return nullptr;
        }
        Asset fragAsset;
        if (assets.find(shader_name + ".frag") != assets.end()) {
            fragAsset = assets[shader_name + ".frag"];
        } else if (assets.find(shader_name + ".fs") != assets.end()) {
            fragAsset = assets[shader_name + ".fs"];
        } else {
            getAsset(shader_name + ".frag");
            return nullptr;
        }
        // convert asset's uint8_t vector to char*
        char *vertData = new char[vertAsset.getData().size()];
        for (int i = 0; i < vertAsset.getData().size(); i++) {
            vertData[i] = static_cast<char>(vertAsset.getData()[i]);
        }
        vertData[vertAsset.getData().size()] = '\0';
        char *fragData = new char[fragAsset.getData().size()];
        for (int i = 0; i < fragAsset.getData().size(); i++) {
            fragData[i] = static_cast<char>(fragAsset.getData()[i]);
        }
        fragData[fragAsset.getData().size()] = '\0';
        EogllShaderProgram *shader = eogllLinkProgram(vertData, fragData);
        shaders[shader_name] = shader;
        // print shader source
        return shader;
    }

    Asset Engine::getAsset(const std::string &asset_name) {
        if (assets.find(asset_name) != assets.end()) {
            return assets[asset_name];
        }
        std::cerr << "Asset not found: " << asset_name << std::endl;
        ErrorPopupWindow("Error", "Asset '" + asset_name + "' not found");
        return {};
    }

}