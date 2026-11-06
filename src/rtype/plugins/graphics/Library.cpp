/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Library
*/

#include "rtype/ecs/app/PluginEntry.hpp"

// Its plugin needs the backends, which come with the next change: the library only has to be one the Engine can open.
RTYPE_PLUGIN_LIBRARY(registrar) { static_cast<void>(registrar); }
