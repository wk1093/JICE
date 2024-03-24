#pragma once

#include <eogll.h>
#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui.h>
#include <imgui_internal.h>
#include <imgui_stdlib.h>
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"

void initUI(EogllWindow* window);

void updateUI();
void drawUI(EogllWindow* window);

void destroyUI();