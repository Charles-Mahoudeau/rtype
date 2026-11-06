/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** LibraryEntry
*/

#include "core/library/LibraryEntry.hpp"

#include <format>
#include <string_view>

#include "core/library/exceptions/LibraryException.hpp"

namespace rtype::core::library {

void checkLibrary(const LibraryEntry& entry, const std::string_view file) {
    if (entry.abiVersion != kLibraryAbiVersion) {
        throw exceptions::LibraryException{
            std::format("Library '{}' was built for the version {}, the engine has {}: rebuild it", file,
                        entry.abiVersion, kLibraryAbiVersion)};
    }
    const std::string_view toolchain = entry.toolchain != nullptr ? entry.toolchain : "";
    if (toolchain != RTYPE_LIBRARY_TOOLCHAIN) {
        throw exceptions::LibraryException{
            std::format("Library '{}' was built with '{}', the engine with '{}': rebuild it", file, toolchain,
                        RTYPE_LIBRARY_TOOLCHAIN)};
    }
}

}  // namespace rtype::core::library
