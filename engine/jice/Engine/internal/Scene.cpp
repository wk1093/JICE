#include "Scene.h"

#include <iostream>


namespace jice {
    Scene::Scene(Engine *e) {
        engine = e;
    }

    void Scene::Setup() {
        for (auto object: children) {
            for (auto attr: object->attributes) {
                if (attr->isScript) {
                    if (attr->script == nullptr) {
                        std::cout << "Script is null" << std::endl;
                        continue;
                    }
                    auto deps = attr->script->getDependencies();
                    for (const auto &dep: deps) {
                        if (!object->hasComponentFromName(dep)) {
                            std::cout << "Missing dependency: " << dep << std::endl;
                        }
                    }
                    attr->script->Setup();
                } else {
                    if (attr->builtin == nullptr) {
                        std::cout << "Builtin is null" << std::endl;
                        continue;
                    }
                    // check dependencies
                    auto deps = attr->builtin->getDependencies();
                    for (const auto &dep: deps) {
                        if (!object->hasComponentFromName(dep)) {
                            std::cout << "Missing dependency: " << dep << std::endl;
                        }
                    }
                    attr->builtin->Update(engine, object);
                }
            }
        }
    }

    void Scene::Update() {
        for (auto object: children) {
            for (auto attr: object->attributes) {
                if (attr->isScript) {
                    if (attr->script == nullptr) {
                        std::cout << "Script is null" << std::endl;
                        continue;
                    }
                    attr->script->Update();
                } else {
                    if (attr->builtin == nullptr) {
                        std::cout << "Builtin is null" << std::endl;
                        continue;
                    }
                    attr->builtin->Update(engine, object);
                }
            }
        }
    }
}