//
// Created by geguj on 2026/2/27.
//

#include "common.h"

volatile bool has_err = false;
std::string P_TARGET =
#if defined(WIN32) || defined(WIN64)
    "Windows"
#elif defined(__APPLE__)
    "MacOS"
#elif defined(__linux__)
    "Linux"
#endif
;   // not set TARGET default is current system
