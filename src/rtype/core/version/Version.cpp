/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Version
*/

#include "core/version/Version.hpp"

#include <string>

namespace rtype::core::version {

std::string describe(const VersionInfo& info) {
    std::string text{"aylo-engine "};

    text += info.version;
    text += "\ncommit: ";
    text += info.commit.empty() ? kUnknownCommit : info.commit;
    text += "\ntoolchain: ";
    text += info.toolchain;
    text += "\nlibrary ABI: ";
    text += std::to_string(info.libraryAbiVersion);
    text += '\n';
    return text;
}

}  // namespace rtype::core::version
