/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** RuntimeConfig
*/

#pragma once

#include <cstdint>

namespace rtype::luau {
/// @brief Configuration of the Luau runtime.
struct RuntimeConfig {
    /// @brief Bit flags selecting the standard Luau libraries to open.
    enum class Libs : std::uint32_t {
        kNone = 0,  ///< No libraries.

        /// @name Standard Luau libraries.
        /// @{
        kMath = 1 << 0,       ///< `math`
        kTable = 1 << 1,      ///< `table`
        kString = 1 << 2,     ///< `string`
        kCoroutine = 1 << 3,  ///< `coroutine`
        kBit32 = 1 << 4,      ///< `bit32`
        kUtf8 = 1 << 5,       ///< `utf8`
        kOs = 1 << 6,         ///< `os`
        kDebug = 1 << 7,      ///< `debug`
        kBuffer = 1 << 8,     ///< `buffer`
        kVector = 1 << 9,     ///< `vector`
        kStandard = kMath | kTable | kString | kCoroutine | kBit32 | kUtf8 | kOs | kDebug | kBuffer |
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
}  // namespace rtype::luau
