#pragma once

#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui.h>
#include <imgui_internal.h>
#include <imgui_stdlib.h>

namespace ImGui {
    void BeginGroupPanel(const char* name, const ImVec2& size = ImVec2(-1.0f, 0.0f));
    bool BeginGroupPanelButton(const char* name, const char* label, const ImVec2& size = ImVec2(-1.0f, 0.0f));
    void EndGroupPanel();
}