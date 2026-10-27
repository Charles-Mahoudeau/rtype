/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** MiniaudioImplementation
*/

// miniaudio's and stb_vorbis's implementations, compiled in this file only. stb_vorbis is declared first so that
// miniaudio enables its OGG Vorbis decoder, then compiled after it.
#define STB_VORBIS_HEADER_ONLY
#include <stb_vorbis.c>

#define MINIAUDIO_IMPLEMENTATION
#include <miniaudio.h>

#undef STB_VORBIS_HEADER_ONLY
#include <stb_vorbis.c>
