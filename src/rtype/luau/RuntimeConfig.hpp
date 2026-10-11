/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** RuntimeConfig
*/

#pragma once

#include <cstdint>

#include "Export.hpp"

namespace rtype::luau {
/// @brief Configuration of the Luau runtime.
struct RuntimeConfig {
    /// @brief Bit flags selecting the standard Luau libraries to open.
    enum class Libs : std::uint16_t {
        kNone = 0,  ///< No libraries.

        /// @name Standard Luau libraries.
        /// @{
        kBase = 1U << 0U,       ///< Base functions (`print`, `pairs`, `pcall`...), registered in the globals.
        kMath = 1U << 1U,       ///< `math`
        kTable = 1U << 2U,      ///< `table`
        kString = 1U << 3U,     ///< `string`
        kCoroutine = 1U << 4U,  ///< `coroutine`
        kBit32 = 1U << 5U,      ///< `bit32`
        kUtf8 = 1U << 6U,       ///< `utf8`
        kOs = 1U << 7U,         ///< `os`
        kDebug = 1U << 8U,      ///< `debug`
        kBuffer = 1U << 9U,     ///< `buffer`
        kVector = 1U << 10U,    ///< `vector`
        // NOLINTNEXTLINE(bugprone-signed-bitwise): enumerators of a 16-bit enum promote to int
        kStandard = kBase | kMath | kTable | kString | kCoroutine | kBit32 | kUtf8 | kOs | kDebug | kBuffer |
                    kVector,  ///< Standard libraries.
        /// @}
    };

    /// @brief Level of optimization applied when compiling Luau scripts.
    enum class OptimizationLevel : std::uint8_t {
        kDisabled = 0,    ///< No optimization.
        kStandard = 1,    ///< Optimizations that preserve debuggability.
        kAggressive = 2,  ///< Optimizations that may impair debugging (e.g. function inlining).
    };

    Libs libs;                            ///< Standard libraries to open (bitwise combination of `Libs` flags).
    OptimizationLevel optimizationLevel;  ///< Optimization level used to compile scripts.
};

/// @brief Combines two sets of libraries.
/// @return The libraries present in @p lhs, @p rhs or both.
[[nodiscard]] RTYPE_LUAU_API RuntimeConfig::Libs operator|(RuntimeConfig::Libs lhs, RuntimeConfig::Libs rhs) noexcept;

/// @brief Intersects two sets of libraries.
/// @return The libraries present in both @p lhs and @p rhs.
[[nodiscard]] RTYPE_LUAU_API RuntimeConfig::Libs operator&(RuntimeConfig::Libs lhs, RuntimeConfig::Libs rhs) noexcept;
}  // namespace rtype::luau
