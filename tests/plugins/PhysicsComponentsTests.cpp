/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** PhysicsComponentsTests
*/

#include <gtest/gtest.h>

#include <array>
#include <cstddef>
#include <glm/ext/vector_float2.hpp>
#include <string_view>
#include <utility>

#include "plugins/physics/Collider.hpp"
#include "plugins/physics/RigidBody.hpp"
#include "plugins/physics/Velocity.hpp"
#include "rtype/ecs/component/ComponentTraits.hpp"
#include "rtype/ecs/type/Field.hpp"

using rtype::ecs::FieldType;
using rtype::plugins::physics::BodyType;
using rtype::plugins::physics::Collider;
using rtype::plugins::physics::RigidBody;
using rtype::plugins::physics::ShapeType;
using rtype::plugins::physics::Velocity;

namespace {
/// @brief Checks the reflected fields of `T`, in order: their names and types.
template <typename T, std::size_t N>
void expectFields(const std::array<std::pair<std::string_view, FieldType>, N>& expected) {
    const auto& fields = rtype::ecs::ComponentTraits<T>::kFields;
    ASSERT_EQ(fields.size(), N);
    for (std::size_t index = 0; index < N; ++index) {
        EXPECT_EQ(fields.at(index).name, expected.at(index).first) << index;
        EXPECT_EQ(fields.at(index).type, expected.at(index).second) << expected.at(index).first;
    }
}
}  // namespace

TEST(RigidBody, ReflectsItsFields) {
    EXPECT_EQ(rtype::ecs::ComponentTraits<RigidBody>::kName, "rtype.RigidBody");
    expectFields<RigidBody, 7>({{{"type", FieldType::kU8},
                                 {"mass", FieldType::kF32},
                                 {"linearDamping", FieldType::kF32},
                                 {"angularDamping", FieldType::kF32},
                                 {"gravityScale", FieldType::kF32},
                                 {"fixedRotation", FieldType::kBool},
                                 {"continuousCollision", FieldType::kBool}}});
}

TEST(RigidBody, DefaultsToADynamicBodyUnderGravity) {
    const RigidBody body;
    EXPECT_EQ(body.type, BodyType::kDynamic);
    EXPECT_FLOAT_EQ(body.mass, 1.0F);
    EXPECT_FLOAT_EQ(body.gravityScale, 1.0F);
    EXPECT_FALSE(body.fixedRotation);
    EXPECT_FALSE(body.continuousCollision);
}

TEST(Collider, ReflectsItsFields) {
    EXPECT_EQ(rtype::ecs::ComponentTraits<Collider>::kName, "rtype.Collider");
    expectFields<Collider, 10>({{{"shape", FieldType::kU8},
                                 {"size", FieldType::kVec2},
                                 {"radius", FieldType::kF32},
                                 {"halfHeight", FieldType::kF32},
                                 {"offset", FieldType::kVec2},
                                 {"sensor", FieldType::kBool},
                                 {"friction", FieldType::kF32},
                                 {"restitution", FieldType::kF32},
                                 {"layer", FieldType::kU32},
                                 {"mask", FieldType::kU32}}});
}

TEST(Collider, DefaultsToASolidCenteredBox) {
    const Collider collider;
    EXPECT_EQ(collider.shape, ShapeType::kBox);
    EXPECT_EQ(collider.size, (glm::vec2{32.0F, 32.0F}));
    EXPECT_EQ(collider.offset, (glm::vec2{0.0F, 0.0F}));
    EXPECT_FALSE(collider.sensor);
}

TEST(Collider, DefaultsToTheFirstCategoryCollidingWithAll) {
    const Collider collider;
    EXPECT_EQ(collider.layer, 1U);
    EXPECT_EQ(collider.mask, rtype::plugins::physics::kAllLayers);
    EXPECT_EQ(rtype::plugins::physics::kAllLayers, 0x7FFFU);
}

TEST(Velocity, ReflectsItsFields) {
    EXPECT_EQ(rtype::ecs::ComponentTraits<Velocity>::kName, "rtype.Velocity");
    expectFields<Velocity, 2>({{{"linear", FieldType::kVec2}, {"angular", FieldType::kF32}}});
}

TEST(Velocity, DefaultsToAtRest) {
    const Velocity velocity;
    EXPECT_EQ(velocity.linear, (glm::vec2{0.0F, 0.0F}));
    EXPECT_FLOAT_EQ(velocity.angular, 0.0F);
}
