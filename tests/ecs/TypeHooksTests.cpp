/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** TypeHooksTests
*/

#include <gtest/gtest.h>

#include <array>
#include <cstddef>
#include <memory>
#include <new>
#include <utility>
#include <vector>

#include "rtype/ecs/type/TypeHooks.hpp"

namespace {
/// @brief Move-only, like the platform or the renderer as a resource.
struct MoveOnly {
    std::unique_ptr<int> value;
};

/// @brief No default constructor: it can only be built from a value.
class NoDefault {
  public:
    explicit NoDefault(int value) : _values{value} {}
    ~NoDefault() = default;
    NoDefault(const NoDefault& other) = default;
    NoDefault& operator=(const NoDefault& other) = default;
    NoDefault(NoDefault&& other) noexcept = default;
    NoDefault& operator=(NoDefault&& other) noexcept = default;

    [[nodiscard]] int get() const { return _values.front(); }

  private:
    std::vector<int> _values;  ///< Not trivially copyable, so every other hook exists.
};

/// @brief Raw storage for one T, as a pool or a resource slot holds it.
template <typename T>
struct Storage {
    alignas(T) std::array<std::byte, sizeof(T)> bytes;

    void* raw() { return bytes.data(); }
    T& get() { return *std::launder(reinterpret_cast<T*>(bytes.data())); }  // NOLINT(*-reinterpret-cast)
};
}  // namespace

TEST(TypeHooks, AMoveOnlyTypeHasNoCopyHook) {
    constexpr rtype::ecs::TypeHooks kHooks = rtype::ecs::detail::makeHooks<MoveOnly>();
    EXPECT_EQ(kHooks.copy, nullptr);
    EXPECT_NE(kHooks.move, nullptr);
    EXPECT_NE(kHooks.destruct, nullptr);
}

TEST(TypeHooks, MovesAndDestroysAMoveOnlyValue) {
    constexpr rtype::ecs::TypeHooks kHooks = rtype::ecs::detail::makeHooks<MoveOnly>();
    MoveOnly source{.value = std::make_unique<int>(7)};
    Storage<MoveOnly> destination{};
    kHooks.move(destination.raw(), &source);
    EXPECT_EQ(*destination.get().value, 7);
    EXPECT_EQ(source.value, nullptr);  // NOLINT(bugprone-use-after-move): moved-from by the hook, on purpose
    kHooks.destruct(destination.raw());
}

TEST(TypeHooks, ATypeWithoutDefaultConstructorHasNoConstructHook) {
    constexpr rtype::ecs::TypeHooks kHooks = rtype::ecs::detail::makeHooks<NoDefault>();
    EXPECT_EQ(kHooks.construct, nullptr);
    EXPECT_NE(kHooks.copy, nullptr);
    EXPECT_NE(kHooks.move, nullptr);
    EXPECT_NE(kHooks.destruct, nullptr);
}

TEST(TypeHooks, CopiesATypeWithoutDefaultConstructor) {
    constexpr rtype::ecs::TypeHooks kHooks = rtype::ecs::detail::makeHooks<NoDefault>();
    const NoDefault source{4};
    Storage<NoDefault> destination{};
    kHooks.copy(destination.raw(), &source);
    EXPECT_EQ(destination.get().get(), 4);
    EXPECT_EQ(source.get(), 4);
    kHooks.destruct(destination.raw());
}
