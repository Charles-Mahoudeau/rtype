/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ConsoleLog
*/

#pragma once

#include <cstddef>
#include <cstdint>
#include <deque>
#include <mutex>
#include <string>
#include <string_view>
#include <vector>

#include "editor/Export.hpp"

namespace rtype::editor::console {

/// @brief How a line of the output reads: an error, a warning or anything else.
enum class Level : std::uint8_t {
    kInfo,     ///< Anything else.
    kWarning,  ///< It says "warning".
    kError,    ///< It says "error".
};

/// @brief One line of the output.
struct Line {
    std::string text;           ///< Without its line end.
    Level level{Level::kInfo};  ///< How it reads.
};

/// @return How @p text reads: an error if it says "error", a warning if it says "warning", in any case.
[[nodiscard]] RTYPE_EDITOR_API Level levelOf(std::string_view text);

/// @brief The last lines the program wrote, for the Console panel: filled from any thread (OutputCapture's), read by
/// the panel.
class RTYPE_EDITOR_API ConsoleLog {
  public:
    static constexpr std::size_t kCapacity = 5000;  ///< Lines kept: the oldest go first.

    ConsoleLog() = default;
    ~ConsoleLog() = default;
    ConsoleLog(const ConsoleLog& other) = delete;
    ConsoleLog& operator=(const ConsoleLog& other) = delete;
    ConsoleLog(ConsoleLog&& other) = delete;
    ConsoleLog& operator=(ConsoleLog&& other) = delete;

    /// @brief Adds the text @p bytes holds, cut into lines: a line not ended yet waits for the rest.
    void write(std::string_view bytes);

    /// @brief Adds one whole line.
    void addLine(std::string_view text);

    /// @return A copy of the lines, the oldest first.
    [[nodiscard]] std::vector<Line> getLines() const;

    /// @return How many lines were ever added: changes when a line comes, so a view knows to scroll.
    [[nodiscard]] std::size_t getAdded() const;

    /// @brief Forgets every line.
    void clear();

  private:
    /// @brief Adds a line, the lock held.
    void push(std::string text);

    mutable std::mutex _mutex;  ///< The output is written from another thread than the panel's.
    std::deque<Line> _lines;    ///< The last kCapacity lines.
    std::string _pending;       ///< The start of a line whose end has not come yet.
    std::size_t _added{0};      ///< Every line ever added.
};

}  // namespace rtype::editor::console
