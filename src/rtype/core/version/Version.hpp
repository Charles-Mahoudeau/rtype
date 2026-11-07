/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Version
*/

#pragma once

#include <cstdint>
#include <string>
#include <string_view>

namespace rtype::core::version {

/// @brief What identifies an engine build: what `aylo-runtime --version` prints and a bug report quotes.
struct VersionInfo {
    std::string_view version;    ///< The engine's version, `0.1.0`, or `0.1.0-nightly.<commit>` for a nightly build.
    std::string_view commit;     ///< The commit it was built from; empty when the build did not know it.
    std::string_view toolchain;  ///< The compiler and standard library it was built with.
    std::uint32_t libraryAbiVersion;  ///< The version of what the engine and its libraries share (kLibraryAbiVersion).
};

/// @brief The word that stands for a commit a build did not know.
inline constexpr std::string_view kUnknownCommit = "unknown";

/// @brief Writes @p info as the output of `--version`: one `key: value` line each, the first being the product and its
/// version, so a script can read it with a plain text search.
/// @code
/// aylo-engine 0.1.0
/// commit: 772b71bd
/// toolchain: clang 21.1.0
/// library ABI: 1
/// @endcode
/// @param info The build.
/// @return The lines, each ending with a newline.
[[nodiscard]] std::string describe(const VersionInfo& info);

}  // namespace rtype::core::version
