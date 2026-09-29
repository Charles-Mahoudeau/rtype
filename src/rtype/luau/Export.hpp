/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Export
*/

#pragma once

#ifdef _WIN32
#ifdef RTYPE_LUAU_BUILD
#define RTYPE_LUAU_API __declspec(dllexport)
#else
#define RTYPE_LUAU_API __declspec(dllimport)
#endif
#else
#define RTYPE_LUAU_API __attribute__((visibility("default")))
#endif
