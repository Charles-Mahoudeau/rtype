/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Runtime
*/

#pragma once

#include <memory>
#include <optional>

struct lua_State;

namespace rtype::luau {
class Runtime {
  public:
    [[nodiscard]] static std::optional<Runtime> create() noexcept;

    ~Runtime() noexcept;
    Runtime(const Runtime& other) noexcept = delete;
    Runtime& operator=(const Runtime& other) noexcept = delete;
    Runtime(Runtime&& other) noexcept;
    Runtime& operator=(Runtime&& other) noexcept;

  private:
    explicit Runtime(lua_State* state) noexcept;

    struct StateDeleter {
        void operator()(lua_State *state) const noexcept;
    };
    std::unique_ptr<lua_State, StateDeleter> _state;
};
}  // namespace rtype::luau
