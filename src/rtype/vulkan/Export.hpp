/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Export
*/

#pragma once

#ifdef _WIN32
#ifdef RTYPE_VULKAN_BUILD
#define RTYPE_VULKAN_API __declspec(dllexport)
#else
#define RTYPE_VULKAN_API __declspec(dllimport)
#endif
#else
#define RTYPE_VULKAN_API __attribute__((visibility("default")))
#endif
