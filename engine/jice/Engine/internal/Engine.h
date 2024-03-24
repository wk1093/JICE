#pragma once
#include <string>
#include "Scene.h"
#include "Object.h"
#include "Scripting.h"
#include "Asset.h"
#include "RenderTask.h"
#include <eogll.h>

#include <functional>
#include <thread>

namespace jice {

    typedef std::function<ScriptInterface *(Engine *e, GameObject *o)> ScriptDispatcher;

    class Engine {
    public:
        EogllWindow* ewindow;
        std::thread splashThread;
        Scene* currentScene = nullptr;

        bool isSplash = false;

        std::unordered_map<std::string, Scene *> scenes;
        std::unordered_map<std::string, ScriptDispatcher> scripts;
        std::unordered_map<std::string, Asset> assets;
        std::vector<RenderTask> renderTasks;
        std::unordered_map<std::string, EogllTexture *> textures;
        std::unordered_map<std::string, EogllShaderProgram *> shaders;
        std::string id;
        std::string name;
        std::string description;
        std::string version;
        std::string author;
        bool isRunning = false;

        Engine(
                std::string id,
                std::string name,
                std::string description,
                std::string version,
                std::string author
        );

        Engine(
                EogllWindow* window,
                EogllFramebuffer* framebuffer
        );

        void beginSplash(const std::string &assetLoc);

        void registerScript(const std::string &scr_name, ScriptDispatcher dispatcher);

        void addAsset(const std::string &name, Asset asset);

        void addScene(const std::string &sc_name, Scene *scene);

        void loadConfig(const std::string &path);

        void endSplash();

        void update();

        void setup();

        void run();

        void close();

        void addRenderTask(const RenderTask &task);

        ScriptInterface *dispatchScript(const std::string &name, GameObject *obj);

        EogllTexture *getTexture(const std::string &tex_name);

        EogllShaderProgram *getShader(const std::string &shader_name);

        Asset getAsset(const std::string &asset_name);

        ~Engine();

    private:
        void keepSplashAlive(const std::string &assetLoc);
    };

}