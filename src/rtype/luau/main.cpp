/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** main
*/

#include <iostream>

#ifdef _WIN32
#define RTYPE_LUAU_API __declspec(dllexport)
#else
#define RTYPE_LUAU_API
#endif

extern "C" RTYPE_LUAU_API void lua_hello() { std::cout << "hello world!" << std::endl; }
