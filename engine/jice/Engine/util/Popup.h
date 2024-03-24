#pragma once

#include <string>
#include "Engine/internal/Asset.h"

// this will halt execution until popup is closed
// since this is an error popup, it will also print the error message to stderr, and then exit the program
void ErrorPopupWindow(const std::string& title, const std::string& message);
