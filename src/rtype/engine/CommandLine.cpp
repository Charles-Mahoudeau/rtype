/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** CommandLine
*/

#include "CommandLine.hpp"

#include <algorithm>
#include <cctype>
#include <charconv>
#include <cstddef>
#include <cstdlib>
#include <format>
#include <functional>
#include <iterator>
#include <map>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>
#include <vector>

#include "rtype/engine/Exceptions.hpp"

namespace {
constexpr std::string_view kFlagPrefix = "--";
constexpr std::string_view kTicksFlag = "--ticks";  ///< The runtime flag of takeFrameLimit().

/// @return Whether @p argument names a flag (`--name`), rather than being a value (`-3` and `-` are values).
bool isFlag(const std::string_view argument) { return argument.starts_with(kFlagPrefix); }

/// @return Whether @p character is a decimal digit.
bool isDigit(const char character) { return std::isdigit(static_cast<unsigned char>(character)) != 0; }

/// @brief Removes the sign @p text starts with, if any.
void skipSign(std::string_view& text) {
    if (text.starts_with('+') || text.starts_with('-')) {
        text.remove_prefix(1);
    }
}

/// @brief Removes the decimal digits @p text starts with.
/// @return How many there were.
std::size_t skipDigits(std::string_view& text) {
    const auto count = static_cast<std::size_t>(std::ranges::find_if_not(text, isDigit) - text.begin());

    text.remove_prefix(count);
    return count;
}

/// @return Whether @p text is a whole decimal number: `[+-]digits[.digits][e[+-]digits]`, with digits on at least one
/// side of the point. Excludes what strtod() also accepts (`inf`, `nan`, hexadecimal) and what it would cut short.
bool isDecimalNumber(std::string_view text) {
    skipSign(text);
    std::size_t digits = skipDigits(text);
    if (text.starts_with('.')) {
        text.remove_prefix(1);
        digits += skipDigits(text);
    }
    if (digits == 0) {
        return false;
    }
    if (text.starts_with('e') || text.starts_with('E')) {
        text.remove_prefix(1);
        skipSign(text);
        if (skipDigits(text) == 0) {
            return false;
        }
    }
    return text.empty();
}
}  // namespace

namespace rtype::engine {
CommandLine CommandLine::parse(const std::span<const std::string> args) {
    CommandLine commandLine;

    for (auto argument = args.begin(); argument != args.end(); ++argument) {
        if (!isFlag(*argument)) {
            throw exceptions::RuntimeError{
                std::format("Unexpected argument '{}': arguments are flags, as in --name value", *argument)};
        }
        const std::string_view flag = *argument;
        std::string_view name = flag.substr(kFlagPrefix.size());
        std::string text{"true"};
        if (const std::size_t equal = name.find('='); equal != std::string_view::npos) {
            text = name.substr(equal + 1);
            name = name.substr(0, equal);
        } else if (const auto next = std::next(argument); next != args.end() && !isFlag(*next)) {
            text = *next;
            argument = next;
        }
        if (name.empty()) {
            throw exceptions::RuntimeError{std::format("Invalid argument '{}': a flag needs a name", flag)};
        }
        commandLine.set(std::string{name}, std::move(text));
    }
    return commandLine;
}

const std::map<std::string, std::string, std::less<>>& CommandLine::flags() const noexcept { return _flags; }

std::optional<std::string> CommandLine::get(const std::string_view name) const {
    if (const auto found = _flags.find(name); found != _flags.end()) {
        return found->second;
    }
    return std::nullopt;
}

void CommandLine::set(std::string name, std::string text) { _flags.insert_or_assign(std::move(name), std::move(text)); }

CommandLine::Value CommandLine::toValue(const std::string_view text) {
    if (text == "true") {
        return true;
    }
    if (text == "false") {
        return false;
    }
    if (isDecimalNumber(text)) {
        const std::string number{text};
        return std::strtod(number.c_str(), nullptr);
    }
    return std::string{text};
}

std::optional<std::size_t> takeFrameLimit(std::vector<std::string>& args) {
    const auto flag = std::ranges::find(args, kTicksFlag);
    if (flag == args.end()) {
        return std::nullopt;
    }
    if (flag + 1 == args.end()) {
        throw exceptions::RuntimeError("--ticks needs a number of frames");
    }
    const std::string value = *(flag + 1);
    std::size_t count = 0;
    const char* const last = std::to_address(value.end());
    const auto [end, error] = std::from_chars(value.data(), last, count);

    if (error == std::errc::result_out_of_range) {
        throw exceptions::RuntimeError(std::format("--ticks is too large: '{}'", value));
    }
    if (error != std::errc{} || end != last) {
        throw exceptions::RuntimeError(std::format("--ticks needs a number of frames, got '{}'", value));
    }
    args.erase(flag, flag + 2);
    if (std::ranges::find(args, kTicksFlag) != args.end()) {
        throw exceptions::RuntimeError("--ticks is given more than once");
    }
    return count;
}
}  // namespace rtype::engine
