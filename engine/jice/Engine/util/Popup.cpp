#include "Popup.h"

#include <iostream>
#include <boxer/boxer.h>

void ErrorPopupWindow(const std::string& title, const std::string& message) {
    std::cerr << message << std::endl;
    boxer::show(message.c_str(), title.c_str(), boxer::Style::Error, boxer::Buttons::OK);
    exit(1);
}

