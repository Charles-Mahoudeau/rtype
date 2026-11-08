/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ScriptPrompt
*/

#pragma once

#include <cstddef>
#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "editor/Export.hpp"
#include "editor/console/ConsoleLog.hpp"
#include "rtype/ecs/entity/Entity.hpp"

namespace rtype::editor::console {

/// @brief What running a line of Luau gave.
struct ScriptOutcome {
    bool ok{false};    ///< False when the line did not compile, raised an error, or there is no script to run.
    std::string text;  ///< The values it gave written as Luau (empty if none), or the error message.
};

/// @brief What a line reaches in the world: the entity `self` stands for, and whether components may be written.
struct ScriptTarget {
    ecs::Entity self;     ///< The selected entity; null: none.
    bool writable{true};  ///< False when the world is a server's copy (a client never writes it).

    friend bool operator==(const ScriptTarget& lhs, const ScriptTarget& rhs) noexcept = default;
};

/// @brief What the prompt runs Luau with: the program gives it (the editor knows no Luau, the engine does).
struct ScriptBackend {
    std::function<ScriptOutcome(std::string_view, const ScriptTarget&)> evaluate;             ///< Runs one line.
    std::function<std::vector<std::string>(std::string_view, const ScriptTarget&)> complete;  ///< The words that
                                                                                              ///< complete a name.
};

/// @brief The input line of the Console: runs what is typed in the game's scripts, remembers it, and completes names.
///
/// @details It knows no ImGui and no Luau: the panel feeds it text, the ScriptBackend runs and completes. It lives in
/// the EditorSession, so the history survives a rebuild of the App.
class RTYPE_EDITOR_API ScriptPrompt {
  public:
    static constexpr std::size_t kHistoryCapacity = 100;  ///< Lines remembered: the oldest go first.
    static constexpr std::size_t kMaxSuggestions = 12;    ///< Completions shown at once.

    ScriptPrompt() = default;
    ~ScriptPrompt() = default;
    ScriptPrompt(const ScriptPrompt& other) = delete;
    ScriptPrompt& operator=(const ScriptPrompt& other) = delete;
    ScriptPrompt(ScriptPrompt&& other) = delete;
    ScriptPrompt& operator=(ScriptPrompt&& other) = delete;

    /// @brief Gives the prompt what runs and completes; without it the Console has no input line.
    void setBackend(ScriptBackend backend);

    /// @return Whether a line can be run.
    [[nodiscard]] bool hasBackend() const noexcept;

    /// @brief Sets what the next lines and completions reach; the kept completions are forgotten if it changed.
    void setTarget(const ScriptTarget& target) noexcept;

    /// @brief Runs @p line, writes `> line` and then what it gave (or its error) in @p log, and remembers it.
    ///
    /// A blank line does nothing. Never throws on a script's account.
    void submit(std::string_view line, ConsoleLog& log);

    /// @return The name at the end of @p text (letters, digits, `_`, `.` and `:`): what a completion replaces.
    [[nodiscard]] static std::string_view wordAt(std::string_view text) noexcept;

    /// @return The longest start all of @p words share; empty if there are none.
    [[nodiscard]] static std::string commonPrefix(const std::vector<std::string>& words);

    /// @brief The words that complete the name at the end of @p text, at most kMaxSuggestions plus how many more.
    ///
    /// @details Asks the backend once for a name: the answer is kept until the name changes or forget() is called.
    /// @param text The text before the cursor.
    /// @param hidden Set to how many completions were left out.
    /// @return The words, whole (`config.player`), sorted; none for an empty name.
    [[nodiscard]] std::vector<std::string> suggestions(std::string_view text, std::size_t& hidden);

    /// @brief Forgets the kept completions: the modules changed (a rebuild, a reload).
    void forget() noexcept;

    /// @brief Goes one line back in the history.
    /// @param current What is typed now, kept to come back to it with newer().
    /// @return The line to show; none if the history is empty or at its oldest line.
    [[nodiscard]] std::optional<std::string> older(std::string_view current);

    /// @brief Goes one line forward in the history.
    /// @return The line to show (the one typed before older() at the end); none if not browsing.
    [[nodiscard]] std::optional<std::string> newer();

  private:
    ScriptTarget _target;                   ///< What the lines reach.
    ScriptBackend _backend;                 ///< What runs and completes.
    std::vector<std::string> _history;      ///< The lines run, the oldest first.
    std::size_t _cursor{0};                 ///< Where browsing is in the history; the size when not browsing.
    std::string _draft;                     ///< What was typed when browsing started.
    std::string _word;                      ///< The name the kept completions are for.
    std::vector<std::string> _completions;  ///< The kept completions of _word.
    bool _hasCompletions{false};            ///< Whether _completions is for _word.
};

}  // namespace rtype::editor::console
