#include "editor_ui.h"

#include <iostream>
#include "menus/Menu.h"
#include <filesystem>
#include <thread>

// cross-platform dll loading
#if defined(_WIN32)
#include <windows.h>
#define LIB_HANDLE HINSTANCE
#define LIB_LOAD(path) LoadLibrary(path)
#define LIB_VERIFY(lib) (lib != nullptr)
#define LIB_GET_FUNC(lib, name) GetProcAddress(lib, name)
#define LIB_CLOSE(lib) FreeLibrary(lib)
#define LIB_PRINT_ERROR() std::cerr << "Error: " << GetLastError() << std::endl
#else
#include <dlfcn.h>
#define LIB_HANDLE void*
#define LIB_LOAD(path) dlopen(path, RTLD_LAZY)
#define LIB_VERIFY(lib) (lib != nullptr)
#define LIB_GET_FUNC(lib, name) dlsym(lib, name)
#define LIB_CLOSE(lib) dlclose(lib)
#define LIB_PRINT_ERROR() std::cerr << "Error: "; printf("%s\n", dlerror())
#endif

typedef int (*MainGameFunc)(bool isCompiled);

void runProject(const std::string& path) {
    auto p = std::filesystem::path(path) / "build" / "out";
    if (!std::filesystem::exists(p)) {
        std::cerr << "Project does not exist" << std::endl;
        return;
    }
    std::filesystem::path libpath;
    if (std::filesystem::exists(p / "libeditor_import.so")) {
        libpath = p / "libeditor_import.so";
    } else if (std::filesystem::exists(p / "editor_import.so")) {
        libpath = p / "editor_import.so";
    } else if (std::filesystem::exists(p / "libeditor_import.dll")) {
        libpath = p / "libeditor_import.dll";
    } else if (std::filesystem::exists(p / "editor_import.dll")) {
        libpath = p / "editor_import.dll";
    } else {
        std::cerr << "Failed to find editor_import library" << std::endl;
        return;
    }

    LIB_HANDLE lib = LIB_LOAD(libpath.string().c_str());
    if (!LIB_VERIFY(lib)) {
        std::cerr << "Failed to load library" << std::endl;
        LIB_PRINT_ERROR();
        return;
    }

    auto libMain = (MainGameFunc)LIB_GET_FUNC(lib, "mainGame");

    std::cout << "libMain" << std::endl;

    if (libMain == nullptr) {
        std::cerr << "Failed to find main function" << std::endl;
        LIB_PRINT_ERROR();
        LIB_CLOSE(lib);
        return;
    }
    // expects us to be in the out directory (for asset loading)
    std::filesystem::path cwd = std::filesystem::current_path();
    std::filesystem::current_path(p);
    libMain(false);
    std::filesystem::current_path(cwd);
    LIB_CLOSE(lib);
}

bool compileProject(const std::string& path) {
    // python {path/to/engine.py} compile {path}
    std::string jice_engine_py = std::getenv("JICE_ENGINE_PY");
    if (jice_engine_py.empty()) {
        std::cerr << "JICE_ENGINE_PY not set" << std::endl;
        return false;
    }
    std::string cmd = "python \"" + jice_engine_py + "\" editor \"" + path + "\"";
    std::cout << cmd << std::endl;
    int ret = system(cmd.c_str());
    return ret == 0;
}

int main() {
    if (eogllInit() != EOGLL_SUCCESS) {
        std::cerr << "Failed to initialize EOGLL" << std::endl;
        return 1;
    }
    EogllWindowHints hints = eogllDefaultWindowHints();
    EogllWindow* window = eogllCreateWindow(1280, 720, "JICE Editor", hints);
    eogllCenterWindow(window);
    glfwSwapInterval(1);

    initUI(window);


    // TODO: project selection
    // TODO: project creation
    // TODO: project compilation

    MenuManager mm;

    JiceProject jp("C:/Users/wyatt/Desktop/structure/projects/testproject/proj.json");


    auto nm = new NavigatorMenu(jp.m_scenes[0]);

    mm.addMenu(new DockspaceMenu());
    mm.addMenu(nm);
    mm.addMenu(new PropertiesMenu(nm, &jp));

    int state = 0;
    std::thread workThread;
    // 1 = saving
    // 2 = compiling
    // 3 = running
    // 4 = done
    // 0 = normal

    bool running = true;

    while (running) {
        updateUI();

        mm.update();
        MenuResult mr = mm.draw();
        if (mr.m_result == MenuResult::Run) {
            workThread = std::thread([&jp, &state](){
                state = 1;
                jp.save();
                state = 2;
                if (!compileProject(jp.m_path)) {
                    std::cerr << "Failed to compile project" << std::endl;
                } else {
                    state = 3;
                    runProject(jp.m_path);
                }
                state = 4;
            });
        }
        if (mr.m_result == MenuResult::Exit) {
            running = false;
        }
        static ImGuiWindowFlags popupFlags = ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize;
        if (state == 1) {
            ImGui::OpenPopup("Saving");
            if (ImGui::BeginPopupModal("Saving", NULL, popupFlags)) {
                ImGui::Text("Saving project...");
                ImGui::EndPopup();
            }
        } else if (state == 2) {
            ImGui::OpenPopup("Compiling");
            if (ImGui::BeginPopupModal("Compiling", NULL, popupFlags)) {
                ImGui::Text("Compiling project...");
                ImGui::EndPopup();
            }
        } else if (state == 3) {
            ImGui::OpenPopup("Running");
            if (ImGui::BeginPopupModal("Running", NULL, popupFlags)) {
                ImGui::Text("Running project...");
                ImGui::EndPopup();
            }
        } else if (state == 4) {
            if (workThread.joinable()) {
                workThread.join();
            }
            state = 0;
        }

        if (eogllWindowShouldClose(window)) {
            // TODO: popup to ask if you want to save the project
            running = false;
        }

        drawUI(window);
    }

    if (workThread.joinable()) {
        workThread.join();
    }

    destroyUI();
    eogllTerminate();
    return 0;
}