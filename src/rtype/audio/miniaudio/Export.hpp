/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Export
*/

#pragma once

/// @brief Marks the classes and functions exported by the rtype-audio-miniaudio library: `RTYPE_AUDIO_MINIAUDIO_BUILD`
/// is defined while building it, so the symbols are exported (`__declspec(dllexport)` on Windows, default visibility
/// elsewhere) and imported by its users. A type with no code of its own, as an interface or an exception, takes
/// RTYPE_HEADER_ONLY_API.
#ifdef _WIN32
#ifdef RTYPE_AUDIO_MINIAUDIO_BUILD
#define RTYPE_AUDIO_MINIAUDIO_API __declspec(dllexport)
#else
#define RTYPE_AUDIO_MINIAUDIO_API __declspec(dllimport)
#endif
#else
#define RTYPE_AUDIO_MINIAUDIO_API __attribute__((visibility("default")))
#endif
