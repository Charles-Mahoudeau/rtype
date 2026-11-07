/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Export
*/

#pragma once

#ifdef _WIN32
#ifdef RTYPE_PLUGIN_UI_BUILD
#define RTYPE_PLUGIN_UI_API __declspec(dllexport)
#else
#define RTYPE_PLUGIN_UI_API __declspec(dllimport)
#endif
#else
#define RTYPE_PLUGIN_UI_API __attribute__((visibility("default")))
#endif
