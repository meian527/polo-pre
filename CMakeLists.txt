cmake_minimum_required(VERSION 3.13)
project(polo-compiler-pre CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /utf-8 /MZ")
endif ()
include_directories(include src)

set(SOURCES
    src/main.cpp
    src/lexer.cpp
    src/parser.cpp
    src/typechecker.cpp
    src/x64/x64gen.cpp
    src/x64/register.h
    src/x64/x64gen.hpp
    src/common.cpp
)
# 创建编译器可执行文件
add_executable(poloc ${SOURCES})
