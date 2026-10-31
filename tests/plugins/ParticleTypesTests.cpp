/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ParticleTypesTests
*/

#include <gtest/gtest.h>

#include <string_view>

#include "plugins/backend/graphics/Particle.hpp"
#include "plugins/particle/ParticleEmitter.hpp"
#include "plugins/particle/ParticleEvents.hpp"
#include "rtype/ecs/component/ComponentTraits.hpp"
#include "rtype/ecs/event/EventTraits.hpp"
#include "rtype/ecs/type/Field.hpp"

using rtype::ecs::FieldType;
using rtype::plugins::particle::ParticleEmitter;
using rtype::plugins::particle::ParticlesFinished;
using rtype::plugins::particle::SpawnParticles;

static_assert(rtype::ecs::Component<ParticleEmitter>);
static_assert(rtype::ecs::Event<SpawnParticles>);
static_assert(rtype::ecs::Event<ParticlesFinished>);

namespace {
/// @return The reflected type of the field @p name of @p T, failing the test if it is not reflected.
template <typename T>
FieldType getFieldType(std::string_view name) {
    for (const auto& field : rtype::ecs::TypeTraits<T>::kFields) {
        if (field.name == name) {
            return field.type;
        }
    }
    ADD_FAILURE() << "No reflected field '" << name << "'";
    return FieldType::kBool;
}
}  // namespace

TEST(ParticleTypesTests, EmitterReflectsEveryField) {
    EXPECT_EQ(rtype::ecs::TypeTraits<ParticleEmitter>::kName, "rtype.ParticleEmitter");
    EXPECT_EQ(rtype::ecs::TypeTraits<ParticleEmitter>::kFields.size(), 7U);
    EXPECT_EQ(getFieldType<ParticleEmitter>("effect"), FieldType::kAssetId);
    EXPECT_EQ(getFieldType<ParticleEmitter>("simulation"), FieldType::kU8);
    EXPECT_EQ(getFieldType<ParticleEmitter>("seed"), FieldType::kU32);
}

TEST(ParticleTypesTests, EmitterDefaultsPlayTheEffectsChoice) {
    const ParticleEmitter emitter;
    EXPECT_TRUE(emitter.playing);
    EXPECT_FLOAT_EQ(emitter.rateScale, 1.0F);
    EXPECT_FALSE(emitter.overrideSimulation);
    EXPECT_EQ(emitter.seed, 0U);
}

TEST(ParticleTypesTests, EventsReflectTheirFields) {
    EXPECT_EQ(rtype::ecs::TypeTraits<SpawnParticles>::kName, "rtype.SpawnParticles");
    EXPECT_EQ(getFieldType<SpawnParticles>("effect"), FieldType::kAssetId);
    EXPECT_EQ(getFieldType<SpawnParticles>("position"), FieldType::kVec2);
    EXPECT_EQ(getFieldType<SpawnParticles>("layer"), FieldType::kI32);
    EXPECT_EQ(rtype::ecs::TypeTraits<ParticlesFinished>::kName, "rtype.ParticlesFinished");
    EXPECT_EQ(getFieldType<ParticlesFinished>("entity"), FieldType::kEntity);
}
