/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** PhysicsSettingsTests
*/

#include <gtest/gtest.h>

#include <utility>
#include <vector>

#include "core/config/Settings.hpp"
#include "core/config/exceptions/ConfigExceptions.hpp"
#include "plugins/physics/PhysicsSettings.hpp"
#include "plugins/physics/debug/PhysicsDebugSettings.hpp"

namespace {
using rtype::core::config::Settings;
using rtype::core::config::exceptions::SettingsException;
using rtype::plugins::physics::PhysicsSettings;
using rtype::plugins::physics::debug::PhysicsDebugSettings;

/// @return A section holding @p key only.
Settings sectionWith(const char* key, Settings::Value value) {
    Settings settings;
    settings.set(key, std::move(value));
    return settings;
}
}  // namespace

TEST(PhysicsSettings, KeepsTheDefaultsOfAnEmptySection) {
    const PhysicsSettings settings = PhysicsSettings::fromSettings(Settings{});
    const PhysicsSettings defaults;
    EXPECT_FLOAT_EQ(settings.gravity.x, defaults.gravity.x);
    EXPECT_FLOAT_EQ(settings.gravity.y, defaults.gravity.y);
    EXPECT_FLOAT_EQ(settings.unitsPerMeter, defaults.unitsPerMeter);
    EXPECT_EQ(settings.maxBodies, defaults.maxBodies);
    EXPECT_EQ(settings.maxBodyPairs, defaults.maxBodyPairs);
    EXPECT_EQ(settings.maxContactConstraints, defaults.maxContactConstraints);
    EXPECT_EQ(settings.tempAllocatorBytes, defaults.tempAllocatorBytes);
}

TEST(PhysicsSettings, ReadsEveryKey) {
    Settings section;
    section.set("gravity", std::vector<double>{0.0, 981.0});
    section.set("unitsPerMeter", 64.0);
    section.set("maxBodies", 128.0);
    section.set("maxBodyPairs", 256.0);
    section.set("maxContactConstraints", 512.0);
    section.set("tempAllocatorBytes", 1048576.0);
    const PhysicsSettings settings = PhysicsSettings::fromSettings(section);
    EXPECT_FLOAT_EQ(settings.gravity.x, 0.0F);
    EXPECT_FLOAT_EQ(settings.gravity.y, 981.0F);
    EXPECT_FLOAT_EQ(settings.unitsPerMeter, 64.0F);
    EXPECT_EQ(settings.maxBodies, 128U);
    EXPECT_EQ(settings.maxBodyPairs, 256U);
    EXPECT_EQ(settings.maxContactConstraints, 512U);
    EXPECT_EQ(settings.tempAllocatorBytes, 1048576U);
}

TEST(PhysicsSettings, RejectsInvalidValues) {
    EXPECT_THROW(static_cast<void>(PhysicsSettings::fromSettings(sectionWith("gravtiy", 1.0))), SettingsException);
    EXPECT_THROW(static_cast<void>(PhysicsSettings::fromSettings(sectionWith("gravity", std::vector<double>{1.0}))),
                 SettingsException);
    EXPECT_THROW(static_cast<void>(PhysicsSettings::fromSettings(sectionWith("gravity", 9.8))), SettingsException);
    EXPECT_THROW(static_cast<void>(PhysicsSettings::fromSettings(sectionWith("unitsPerMeter", 0.0))),
                 SettingsException);
    EXPECT_THROW(static_cast<void>(PhysicsSettings::fromSettings(sectionWith("maxBodies", 0.0))), SettingsException);
    EXPECT_THROW(static_cast<void>(PhysicsSettings::fromSettings(sectionWith("maxBodyPairs", 10.5))),
                 SettingsException);
    EXPECT_THROW(static_cast<void>(PhysicsSettings::fromSettings(sectionWith("maxContactConstraints", 5e9))),
                 SettingsException);
}

TEST(PhysicsDebugSettings, ReadsItsKeysAndKeepsTheOthers) {
    Settings section;
    section.set("enabled", false);
    section.set("drawAll", true);
    section.set("outlineThickness", 3.0);
    section.set("velocityScale", 0.5);
    const PhysicsDebugSettings settings = PhysicsDebugSettings::fromSettings(section);
    const PhysicsDebugSettings defaults;
    EXPECT_FALSE(settings.enabled);
    EXPECT_TRUE(settings.drawAll);
    EXPECT_FLOAT_EQ(settings.outlineThickness, 3.0F);
    EXPECT_FLOAT_EQ(settings.velocityScale, 0.5F);
    EXPECT_EQ(settings.toggleKey, defaults.toggleKey);
}

TEST(PhysicsDebugSettings, RejectsInvalidValues) {
    EXPECT_THROW(static_cast<void>(PhysicsDebugSettings::fromSettings(sectionWith("color", 1.0))), SettingsException);
    EXPECT_THROW(static_cast<void>(PhysicsDebugSettings::fromSettings(sectionWith("enabled", 1.0))), SettingsException);
    EXPECT_THROW(static_cast<void>(PhysicsDebugSettings::fromSettings(sectionWith("outlineThickness", -1.0))),
                 SettingsException);
}
