/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** TypeRegistryTests
*/

#include <gtest/gtest.h>

#include <cstdint>
#include <format>
#include <string>
#include <string_view>
#include <tl/expected.hpp>
#include <utility>

#include "rtype/ecs/error/Error.hpp"
#include "rtype/ecs/error/Result.hpp"
#include "rtype/ecs/type/TypeInfo.hpp"
#include "rtype/ecs/type/TypeRegistry.hpp"
#include "rtype/ecs/type/TypeTraits.hpp"

namespace rtype::test::type_registry {
/// @brief A second kind of type, defined only here, to show the registry does not depend on components.
struct GadgetInfo {
    rtype::ecs::TypeInfo type;  ///< What every kind shares.
    std::int32_t charges{0};    ///< This kind's own option.
};

/// @brief What is chosen when registering a C++ gadget.
struct GadgetOptions {
    std::int32_t charges{0};  ///< Number of charges.
};
}  // namespace rtype::test::type_registry

namespace gadget = rtype::test::type_registry;

template <>
struct rtype::ecs::KindTraits<gadget::GadgetInfo> {
    using Options = gadget::GadgetOptions;
    static constexpr TypeKind kKind = TypeKind::kResource;  // any kind but components works for this test
    static constexpr std::string_view kName = "gadget";
    static constexpr std::string_view kLabel = "Gadget";

    [[nodiscard]] static Result<void> validate(const gadget::GadgetInfo& info) {
        if (Result<void> valid = validateType(info.type, kName); !valid) {
            return valid;
        }
        if (info.charges < 0) {
            return fail(ErrorKind::kInvalidType, std::format("Gadget '{}': negative charges", info.type.name));
        }
        return {};
    }

    [[nodiscard]] static Result<void> checkSameOptions(const gadget::GadgetInfo& registered,
                                                       const gadget::GadgetInfo& requested) {
        if (registered.charges != requested.charges) {
            return tl::unexpected<Error>{detail::optionsMismatch(kLabel, requested.type.name, "charges",
                                                                 std::to_string(registered.charges),
                                                                 std::to_string(requested.charges))};
        }
        return {};
    }

    template <typename T>
    [[nodiscard]] static gadget::GadgetInfo make(const Options& options) {
        return gadget::GadgetInfo{.type = TypeTraits<T>::makeTypeInfo(), .charges = options.charges};
    }
};

namespace {
using GadgetRegistry = rtype::ecs::TypeRegistry<gadget::GadgetInfo>;

/// @return A valid run-time gadget.
gadget::GadgetInfo makeLamp(std::int32_t charges = 3) {
    return gadget::GadgetInfo{.type = {.name = "test.Lamp", .size = 4, .alignment = 4}, .charges = charges};
}
}  // namespace

TEST(TypeRegistry, RegistersAndFindsAnotherKind) {
    GadgetRegistry registry;
    const rtype::ecs::Result<rtype::ecs::TypeId> id = registry.registerType(makeLamp());
    ASSERT_TRUE(id);
    EXPECT_EQ(*id, 0U);
    EXPECT_EQ(registry.getCount(), 1U);
    const gadget::GadgetInfo* lamp = registry.findByName("test.Lamp");
    ASSERT_NE(lamp, nullptr);
    EXPECT_EQ(lamp->charges, 3);
    EXPECT_EQ(lamp->type.layoutHash, rtype::ecs::computeLayoutHash(lamp->type));  // set by the registry
    EXPECT_EQ(registry.findById(*id), lamp);
}

TEST(TypeRegistry, RegisteringTheSameTypeAgainReturnsItsId) {
    GadgetRegistry registry;
    const rtype::ecs::TypeId first = *registry.registerType(makeLamp());
    EXPECT_EQ(*registry.registerType(makeLamp()), first);
    EXPECT_EQ(registry.getCount(), 1U);
}

TEST(TypeRegistry, AppliesTheKindsOwnRules) {
    GadgetRegistry registry;
    const rtype::ecs::Result<rtype::ecs::TypeId> id = registry.registerType(makeLamp(-1));
    ASSERT_FALSE(id);
    EXPECT_EQ(id.error().getKind(), rtype::ecs::ErrorKind::kInvalidType);
    EXPECT_EQ(registry.getCount(), 0U);
}

TEST(TypeRegistry, ComparesTheKindsOwnOptions) {
    GadgetRegistry registry;
    ASSERT_TRUE(registry.registerType(makeLamp(3)));
    const rtype::ecs::Result<rtype::ecs::TypeId> id = registry.registerType(makeLamp(5));
    ASSERT_FALSE(id);
    EXPECT_EQ(id.error().getKind(), rtype::ecs::ErrorKind::kOptionsMismatch);
    EXPECT_EQ(std::string{id.error().getMessage()},
              "Gadget 'test.Lamp' is already registered with charges '3', not '5'");
}

TEST(TypeRegistry, SharedChecksNameTheKind) {
    GadgetRegistry registry;
    ASSERT_TRUE(registry.registerType(makeLamp()));
    gadget::GadgetInfo bigger = makeLamp();
    bigger.type.size = 8;
    const rtype::ecs::Result<rtype::ecs::TypeId> id = registry.registerType(std::move(bigger));
    ASSERT_FALSE(id);
    EXPECT_EQ(id.error().getKind(), rtype::ecs::ErrorKind::kLayoutMismatch);
    EXPECT_NE(std::string{id.error().getMessage()}.find("Gadget 'test.Lamp'"), std::string::npos);
}
