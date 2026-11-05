/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** CommandLine
*/

#pragma once

#include <functional>
#include <map>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <variant>

#include "Export.hpp"

namespace rtype::engine {
/// @brief The flags an executable was started with: `--name value`, `--name=value` or a bare `--name`.
///
/// @details The engine reads the flags it knows (`--game`); the project script reads every flag as `engine.args`
/// (SettingsLoader::setArgs()), so a game decides its own command line:
/// @code{.lua}
/// engine.settings.network.port = engine.args.port or 4242
/// @endcode
/// A bare flag (`--verbose`, or a flag followed by another flag) is `true`. A flag given twice keeps its last value,
/// so a default can be put first and overridden by what follows.
class RTYPE_ENGINE_API CommandLine {
  public:
    /// @brief A flag's value, as scripts see it: see toValue().
    using Value = std::variant<bool, double, std::string>;

    /// @brief Reads flags from the arguments of `main`, without the program name.
    /// @param args The arguments, in order.
    /// @return The flags.
    /// @throws exceptions::RuntimeError If an argument is not a flag nor the value of one, or a flag has no name.
    [[nodiscard]] static CommandLine parse(std::span<const std::string> args);

    CommandLine() = default;
    ~CommandLine() = default;
    CommandLine(const CommandLine& other) = default;
    CommandLine& operator=(const CommandLine& other) = default;
    CommandLine(CommandLine&& other) noexcept = default;
    CommandLine& operator=(CommandLine&& other) noexcept = default;

    /// @return Every flag, by name, as written (a bare flag holds "true").
    [[nodiscard]] const std::map<std::string, std::string, std::less<>>& flags() const noexcept;

    /// @return The text of a flag, as written, or nothing if it was not given.
    [[nodiscard]] std::optional<std::string> get(std::string_view name) const;

    /// @brief Sets a flag, replacing its previous value.
    void set(std::string name, std::string text);

    /// @brief Turns a flag's text into the value scripts see: `true` and `false` are booleans, a decimal number is a
    /// number, anything else stays a string.
    /// @param text The text given after the flag.
    /// @return The value.
    [[nodiscard]] static Value toValue(std::string_view text);

  private:
    std::map<std::string, std::string, std::less<>> _flags;  ///< The text of each flag, by name.
};
}  // namespace rtype::engine
