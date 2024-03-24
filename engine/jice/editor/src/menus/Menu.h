#pragma once

#include <utility>
#include <vector>
#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui.h>
#include <imgui_internal.h>
#include <imgui_stdlib.h>
#include <string>
#include <algorithm>
#include "editor/src/proj/JiceProject.h"

struct MenuResult {
    enum {
        None, // success, no action
        Close, // close the menu, set m_isActive to false
        Error, // error occurred
        Exit, // exit the program
        Run, // run the project
    } m_result;
    std::string m_errorMessage;
};

class MenuManager;

class Menu {
public:
    bool m_isActive;
    std::string m_menuName;

    explicit Menu(std::string name, bool isActive = true) : m_menuName(std::move(name)), m_isActive(isActive) {}

    virtual MenuResult draw(MenuManager* mm) = 0;
    virtual void update(MenuManager* mm) {}

    virtual ~Menu() = default;
};

class MenuManager {
public:
    std::vector<Menu*> m_menus;

    MenuManager() = default;

    void addMenu(Menu* menu) {
        m_menus.push_back(menu);
    }

    MenuResult draw() {
        for (auto menu : m_menus) {
            if (menu->m_isActive) {
                auto result = menu->draw(this);
                if (result.m_result == MenuResult::Close) {
                    menu->m_isActive = false;
                } else if (result.m_result != MenuResult::None) {
                    return result;
                }
            }
        }
        return {MenuResult::None};
    }

    void update() {
        for (auto menu : m_menus) {
            if (menu->m_isActive) {
                menu->update(this);
            }
        }
    }

    void cleanup() {
        for (auto menu : m_menus) {
            delete menu;
        }
    }

    ~MenuManager() {
        cleanup();
    }
};

class DockspaceMenu : public Menu {
public:
    DockspaceMenu() : Menu("dockspace", true) {}

    MenuResult draw(MenuManager* mm) override {
        static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_None;
        ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
        const ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(viewport->WorkPos);
        ImGui::SetNextWindowSize(viewport->WorkSize);
        ImGui::SetNextWindowViewport(viewport->ID);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
        window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
        window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
        window_flags |= ImGuiWindowFlags_NoBackground;
        ImGui::Begin("DockSpace", nullptr, window_flags);
        ImGui::PopStyleVar(2);
        ImGuiIO& io = ImGui::GetIO();
        if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable) {
            ImGuiID dockspace_id = ImGui::GetID("jiceengine");
            ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);
        } else {
            ImGui::Text("ERROR: Docking is not enabled. Please enable it in the ImGui config flags.");
        }

        if (ImGui::BeginMenuBar()) {
            if (ImGui::BeginMenu("File")) {
                if (ImGui::MenuItem("Exit")) {
                    ImGui::EndMenu();
                    ImGui::EndMenuBar();
                    ImGui::End();
                    return {MenuResult::Exit};
                }
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("View")) {

                // display a checkbox for each menu in mm (other than self) to toggle visibility
                for (auto menu : mm->m_menus) {
                    if (menu != this) {
                        if (ImGui::MenuItem((menu->m_isActive ? "Hide " + menu->m_menuName : "Show " + menu->m_menuName).c_str(), nullptr, menu->m_isActive)) {
                            menu->m_isActive = !menu->m_isActive;
                        }
                    }
                }

                ImGui::EndMenu();
            }
            ImGui::EndMenuBar();
        }
        ImGui::End();
        return {MenuResult::None};
    }
};


class NavigatorMenu : public Menu {
public:
    JiceScene& m_scene;
    std::vector<JiceObject*> m_selectedObjects;
    explicit NavigatorMenu(JiceScene& scene) : m_scene(scene), Menu("Explorer") {}

    void draw_object(JiceObject* obj) {
        // if object has no children, draw as selectable, else draw as tree node
        // the tree node should only expand if the arrow is clicked, if the object is clicked, it should be selected
        static ImGuiTreeNodeFlags base_flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_SpanAvailWidth;
        // if selected
        ImGuiTreeNodeFlags flags = base_flags;
        if (std::find(m_selectedObjects.begin(), m_selectedObjects.end(), obj) != m_selectedObjects.end()) {
            flags |= ImGuiTreeNodeFlags_Selected;
        }

        bool clicked = false;
        bool node_open = false;
        if (!obj->m_children.empty()) {
            node_open = ImGui::TreeNodeEx(obj->m_id.c_str(), flags);
            if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen()) {
                clicked = true;
            }
        } else {
            flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
            ImGui::TreeNodeEx(obj->m_id.c_str(), flags);
            if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen()) {
                clicked = true;
            }

        }
        if (ImGui::BeginDragDropTarget()) {
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("JiceObject")) {
                IM_ASSERT(payload->DataSize == sizeof(JiceObject));
                auto* pay_obj = (JiceObject*)payload->Data;
                if (pay_obj == nullptr) {
                    std::cout << "dropped null object" << std::endl;
                    ImGui::EndDragDropTarget();
                    goto drag_drop_end;
                }
                if (obj->m_id == pay_obj->m_id) {
                    std::cout << "dropped onto self" << std::endl;
                    ImGui::EndDragDropTarget();
                    goto drag_drop_end;
                }
                if (obj->m_id.empty() || pay_obj->m_id.empty()) {
                    std::cout << "dropped object has no id" << std::endl;
                    ImGui::EndDragDropTarget();
                    goto drag_drop_end;
                }
                // we can't drop an object onto one of its children
                if (isChildOf(pay_obj, obj)) {
                    std::cout << "dropped object is child of target" << std::endl;
                    ImGui::EndDragDropTarget();
                    goto drag_drop_end;
                }

                // remove from old parent
                if (!removeJiceObjectFromScene(m_scene, pay_obj)) {
                    std::cout << "failed to remove from old parent" << std::endl;
                    ImGui::EndDragDropTarget();
                    goto drag_drop_end;
                }

                // causes a bug where the object added is invalid
                // this is because the object is deleted
                m_selectedObjects.clear();
                auto* new_obj = new JiceObject(*pay_obj);
                obj->m_children.push_back(new_obj);




                ImGui::EndDragDropTarget();
            }
        }
        drag_drop_end:
        if (ImGui::BeginDragDropSource()) {
            ImGui::SetDragDropPayload("JiceObject", obj, sizeof(JiceObject));
            ImGui::Text("%s", obj->m_id.c_str());
            ImGui::EndDragDropSource();
        }
        if (node_open) {
            for (auto& child : obj->m_children) {
                draw_object(child);
            }
            ImGui::TreePop();
        }
        if (clicked) {
            if (ImGui::GetIO().KeyCtrl) {
                // if ctrl is held, add to selection
                m_selectedObjects.push_back(obj);
            } else {
                // if not, clear selection and select this object
                m_selectedObjects.clear();
                m_selectedObjects.push_back(obj);

            }
        }
    }

    MenuResult draw(MenuManager* mm) override {
        ImGui::Begin("Explorer", &m_isActive);
        ImGui::Text("%s", m_scene.m_name.c_str());
        ImGui::SameLine();
        if (ImGui::Button("New Object")) {
            // find available name starting with "Object" then "Object1" etc
            std::string name = "Object";
            int i = 0;
            while (true) {
                bool found = false;
                for (auto& obj : m_scene.m_objects) {
                    if (obj->m_id == name) {
                        found = true;
                        break;
                    }
                }
                if (!found) {
                    break;
                }
                name = "Object" + std::to_string(++i);
            }
            m_scene.m_objects.push_back(new JiceObject(name));
        }

        // tree of objects, with dropdown arrows to expand children
        // objects are SELECTABLE, and when selected, the object is highlighted in the scene view
        for (auto& obj : m_scene.m_objects) {
            draw_object(obj);
        }
        // rectangle to drop objects onto
        ImGui::PushID("drop");
        // make drop target as big as a tree node, but no graphics
        // fill x direction
        ImGui::Dummy(ImVec2(ImGui::GetContentRegionAvail().x, ImGui::GetColumnWidth()));
        // dragging object onto the navigator (from a parent) should take the object and make it a child of the scene
        if (ImGui::BeginDragDropTarget()) {
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("JiceObject")) {
                IM_ASSERT(payload->DataSize == sizeof(JiceObject));
                auto* pay_obj = (JiceObject*)payload->Data;
                if (pay_obj == nullptr) {
                    std::cout << "dropped null object" << std::endl;
                    ImGui::EndDragDropTarget();
                    goto drag_drop_end;
                }
                if (pay_obj->m_id.empty()) {
                    std::cout << "dropped object has no id" << std::endl;
                    ImGui::EndDragDropTarget();
                    goto drag_drop_end;
                }
                // remove from old parent
                if (!removeJiceObjectFromScene(m_scene, pay_obj)) {
                    std::cout << "failed to remove from old parent" << std::endl;
                    ImGui::EndDragDropTarget();
                    goto drag_drop_end;
                }

                m_selectedObjects.clear();
                auto* new_obj = new JiceObject(*pay_obj);
                m_scene.m_objects.push_back(new_obj);

                ImGui::EndDragDropTarget();
            }
        }
        drag_drop_end:
        ImGui::PopID();

        ImGui::End();
        return {MenuResult::None};
    }
};

class PropertiesMenu : public Menu {
public:
    NavigatorMenu* m_navigator;
    JiceProject* m_project;
    explicit PropertiesMenu(NavigatorMenu* navigator, JiceProject* jp) : Menu("Properties"), m_navigator(navigator), m_project(jp) {}

    MenuResult draw(MenuManager* mm) override {
        ImGui::Begin("Properties", &m_isActive);
        if (m_navigator->m_selectedObjects.empty()) {
            ImGui::Text("No object selected");
        } else {
            int i = 0;
            for (auto& obj : m_navigator->m_selectedObjects) {
                if (i++ > 0) {
                    // padding - seperator - padding
                    ImGui::Dummy(ImVec2(0, 5));
                    ImGui::Separator();
                    ImGui::Dummy(ImVec2(0, 5));
                }
                ImGui::Text("Name: ");
                ImGui::SameLine();
                std::string name = obj->m_id;
                ImGui::InputText("##name", &name);
                // verify name is unique and non-empty
                if (name.empty()) {
                    ImGui::TextColored(ImVec4(1, 0, 0, 1), "Name cannot be empty");
                } else {
                    bool name_unique = true;
                    for (auto& scene_obj : m_navigator->m_scene.m_objects) {
                        if (scene_obj == obj) {
                            continue;
                        }
                        if (scene_obj->m_id == name) {
                            name_unique = false;
                            break;
                        }
                    }
                    if (!name_unique) {
                        ImGui::TextColored(ImVec4(1, 0, 0, 1), "Name must be unique");
                    } else {
                        obj->m_id = name;
                    }
                }
                // TODO: delete
                int ind = 0;
                for (auto& attr : obj->m_attrs) {
                    drawAttribute(attr, obj, ind++);
                }

                if (ImGui::Button("Add Attribute")) {
                    ImGui::OpenPopup("Add Attribute");
                }
                ImGui::SameLine();
                if (ImGui::BeginPopup("Add Attribute")) {
                    if (ImGui::MenuItem("Transform")) {
                        if (!obj->hasAttribute("transform")) {
                            obj->m_attrs.push_back(JiceAttribute::Builtin("transform"));
                        }
                    }
                    if (ImGui::MenuItem("Image2d")) {
                        if (!obj->hasAttribute("image2d")) {
                            obj->m_attrs.push_back(JiceAttribute::Builtin("image2d"));
                        }
                    }
                    // loop through files in scripts directory
                    // if file is a script, add it to the menu
                    std::filesystem::path scr_path = m_project->m_scriptLoc;
                    // if scr_path is not absolute, use the project path as the base
                    if (!scr_path.is_absolute()) {
                        scr_path = m_project->m_path / scr_path;
                    }
                    for (const auto& entry : std::filesystem::directory_iterator(scr_path)) {
                        if (entry.path().extension() == ".cpp" || entry.path().extension() == ".c++" || entry.path().extension() == ".cc") {
                            // name is file without extension
                            std::string name = entry.path().filename().replace_extension("").string();
                            if (ImGui::MenuItem(name.c_str())) {
                                if (!obj->hasAttribute(name)) {
                                    obj->m_attrs.push_back(JiceAttribute::Script(name));
                                }
                            }
                        }
                    }
                    ImGui::EndPopup();
                }
            }
        }
        ImGui::End();
        return {MenuResult::None};
    }
};
