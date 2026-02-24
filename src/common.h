//
// Created by meian on 2026/2/23.
//

#ifndef POLO_COMPILER_PRE_COMMON_H
#define POLO_COMPILER_PRE_COMMON_H
#include <iostream>
#include <string>
static volatile bool has_err = false;
static std::string P_TARGET =
#if defined(WIN32) || defined(WIN64)
    "Windows"
#elif defined(__APPLE__)
    "MacOS"
#elif defined(__linux__)
    "Linux"
#endif
;   // not set TARGET default is current system

// 带位置信息的错误报告工具函数
inline void make_error(const std::string& message, size_t line, size_t col) {
    has_err = true;
    std::cerr <<
    ("Error: " + message + " (" + std::to_string(line) + ":" + std::to_string(col) + ")") << std::endl;
}

#define THROW_ERROR(msg, line, col) make_error(msg, line, col)

#endif //POLO_COMPILER_PRE_COMMON_H