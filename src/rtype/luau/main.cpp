/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** main
*/

#include <iostream>

#include "Export.hpp"

extern "C" RTYPE_LUAU_API void lua_hello() { std::cout << "hello world!" << std::endl; }
