/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Runtime
*/

#pragma once

#include <memory>

#include "Export.hpp"
#include "Result.hpp"
#include "RuntimeConfig.hpp"

struct lua_State;

namespace rtype::luau {
class RTYPE_LUAU_API Runtime {
  public:
    [[nodiscard]] static Result<Runtime> create(RuntimeConfig config) noexcept;

    ~Runtime() noexcept;
    Runtime(const Runtime& other) noexcept = delete;
    Runtime& operator=(const Runtime& other) noexcept = delete;
    Runtime(Runtime&& other) noexcept;
    Runtime& operator=(Runtime&& other) noexcept;

  private:
    struct StateDeleter {
        void operator()(lua_State* state) const noexcept;
    };

    explicit Runtime(lua_State* state, RuntimeConfig config) noexcept;

    RuntimeConfig _config;
    std::unique_ptr<lua_State, StateDeleter> _state;
};
}  // namespace rtype::luau
