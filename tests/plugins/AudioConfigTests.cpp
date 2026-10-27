/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** AudioConfigTests
*/

#include <gtest/gtest.h>

#include "plugins/audio/AudioConfig.hpp"
#include "plugins/config/Settings.hpp"
#include "plugins/config/exceptions/ConfigExceptions.hpp"

TEST(AudioConfig, KeepsTheDefaultsOfAnEmptySection) {
    const rtype::plugins::audio::AudioConfig config =
        rtype::plugins::audio::AudioConfig::fromSettings(rtype::plugins::config::Settings{});
    EXPECT_TRUE(config.backend.empty());
    EXPECT_FALSE(config.required);
    EXPECT_EQ(config.maxVoices, 64U);
    EXPECT_FLOAT_EQ(config.volume.master, 1.0F);
    EXPECT_FLOAT_EQ(config.volume.music, 1.0F);
    EXPECT_FLOAT_EQ(config.volume.effects, 1.0F);
    EXPECT_FLOAT_EQ(config.panDistance, 400.0F);
}

TEST(AudioConfig, ReadsEveryKey) {
    rtype::plugins::config::Settings settings;
    settings.set("backend", "miniaudio");
    settings.set("required", true);
    settings.set("maxVoices", 32.0);
    settings.set("volume.master", 0.5);
    settings.set("volume.music", 0.7);
    settings.set("volume.effects", 0.9);
    settings.set("panDistance", 250.0);
    const rtype::plugins::audio::AudioConfig config = rtype::plugins::audio::AudioConfig::fromSettings(settings);
    EXPECT_EQ(config.backend, "miniaudio");
    EXPECT_TRUE(config.required);
    EXPECT_EQ(config.maxVoices, 32U);
    EXPECT_FLOAT_EQ(config.volume.master, 0.5F);
    EXPECT_FLOAT_EQ(config.volume.music, 0.7F);
    EXPECT_FLOAT_EQ(config.volume.effects, 0.9F);
    EXPECT_FLOAT_EQ(config.panDistance, 250.0F);
}

TEST(AudioConfig, RejectsAnUnknownKey) {
    rtype::plugins::config::Settings settings;
    settings.set("maxVoice", 32.0);
    EXPECT_THROW(static_cast<void>(rtype::plugins::audio::AudioConfig::fromSettings(settings)),
                 rtype::plugins::config::exceptions::SettingsException);
}

TEST(AudioConfig, RejectsAnUnknownVolume) {
    rtype::plugins::config::Settings settings;
    settings.set("volume.voice", 1.0);
    EXPECT_THROW(static_cast<void>(rtype::plugins::audio::AudioConfig::fromSettings(settings)),
                 rtype::plugins::config::exceptions::SettingsException);
}

TEST(AudioConfig, RejectsAWrongType) {
    rtype::plugins::config::Settings settings;
    settings.set("backend", 3.0);
    EXPECT_THROW(static_cast<void>(rtype::plugins::audio::AudioConfig::fromSettings(settings)),
                 rtype::plugins::config::exceptions::SettingsException);
}

TEST(AudioConfig, RejectsANegativeVolume) {
    rtype::plugins::config::Settings settings;
    settings.set("volume.music", -0.1);
    EXPECT_THROW(static_cast<void>(rtype::plugins::audio::AudioConfig::fromSettings(settings)),
                 rtype::plugins::config::exceptions::SettingsException);
}

TEST(AudioConfig, RejectsAnInvalidVoiceCount) {
    for (const double maxVoices : {0.0, -4.0, 2.5}) {
        rtype::plugins::config::Settings settings;
        settings.set("maxVoices", maxVoices);
        EXPECT_THROW(static_cast<void>(rtype::plugins::audio::AudioConfig::fromSettings(settings)),
                     rtype::plugins::config::exceptions::SettingsException)
            << maxVoices;
    }
}

TEST(AudioConfig, RejectsANonPositivePanDistance) {
    rtype::plugins::config::Settings settings;
    settings.set("panDistance", 0.0);
    EXPECT_THROW(static_cast<void>(rtype::plugins::audio::AudioConfig::fromSettings(settings)),
                 rtype::plugins::config::exceptions::SettingsException);
}
