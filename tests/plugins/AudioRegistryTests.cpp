/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** AudioRegistryTests
*/

#include <gtest/gtest.h>

#include <memory>
#include <string>
#include <vector>

#include "FakeAudioDevice.hpp"
#include "plugins/audio/AudioConfig.hpp"
#include "plugins/audio/AudioRegistry.hpp"
#include "plugins/audio/IAudioDevice.hpp"
#include "plugins/audio/NullAudioDevice.hpp"
#include "plugins/audio/exceptions/AudioExceptions.hpp"
#include "plugins/config/Settings.hpp"
#include "plugins/config/exceptions/ConfigExceptions.hpp"

TEST(AudioRegistry, AlwaysHasTheNullBackend) {
    const rtype::plugins::audio::AudioRegistry registry;
    EXPECT_EQ(registry.getNames(), std::vector<std::string>{"null"});
    rtype::plugins::config::Settings settings;
    settings.set("audio.backend", "null");
    const std::unique_ptr<rtype::plugins::audio::IAudioDevice> device = registry.createDevice(settings);
    EXPECT_NE(dynamic_cast<rtype::plugins::audio::NullAudioDevice*>(device.get()), nullptr);
}

TEST(AudioRegistry, ListsTheNamesSorted) {
    rtype::plugins::audio::AudioRegistry registry;
    registry.add("zeta", nullptr);
    registry.add("alpha", nullptr);
    EXPECT_EQ(registry.getNames(), (std::vector<std::string>{"alpha", "null", "zeta"}));
}

TEST(AudioRegistry, CreatesTheBackendTheConfigNames) {
    const auto state = std::make_shared<rtype::test::plugins::FakeAudioState>();
    const auto registry = rtype::test::plugins::makeFakeAudioRegistry(state);
    const std::unique_ptr<rtype::plugins::audio::IAudioDevice> device =
        registry->createDevice(rtype::test::plugins::makeFakeAudioSettings());
    EXPECT_NE(dynamic_cast<rtype::test::plugins::FakeAudioDevice*>(device.get()), nullptr);
}

TEST(AudioRegistry, HandsTheFactoryItsSectionAndTheCommonConfig) {
    rtype::plugins::audio::AudioRegistry registry;
    double seenRate = 0.0;
    unsigned seenVoices = 0;
    registry.add(
        "custom",
        [&seenRate, &seenVoices](
            const rtype::plugins::config::Settings& section,
            const rtype::plugins::audio::AudioConfig& config) -> std::unique_ptr<rtype::plugins::audio::IAudioDevice> {
            seenRate = section.getNumber("sampleRate", 0.0);
            seenVoices = config.maxVoices;
            return std::make_unique<rtype::plugins::audio::NullAudioDevice>();
        });
    rtype::plugins::config::Settings settings;
    settings.set("audio.backend", "custom");
    settings.set("audio.maxVoices", 16.0);
    settings.set("custom.sampleRate", 48000.0);
    static_cast<void>(registry.createDevice(settings));
    EXPECT_DOUBLE_EQ(seenRate, 48000.0);
    EXPECT_EQ(seenVoices, 16U);
}

TEST(AudioRegistry, ThrowsWhenTheConfigChoosesNoBackend) {
    const rtype::plugins::audio::AudioRegistry registry;
    EXPECT_THROW(static_cast<void>(registry.createDevice(rtype::plugins::config::Settings{})),
                 rtype::plugins::audio::exceptions::AudioBackendException);
}

TEST(AudioRegistry, ThrowsForAnUnknownBackend) {
    const rtype::plugins::audio::AudioRegistry registry;
    rtype::plugins::config::Settings settings;
    settings.set("audio.backend", "fmod");
    EXPECT_THROW(static_cast<void>(registry.createDevice(settings)),
                 rtype::plugins::audio::exceptions::AudioBackendException);
}

TEST(AudioRegistry, FallsBackToNullWhenTheDeviceCannotOpen) {
    const auto state = std::make_shared<rtype::test::plugins::FakeAudioState>();
    state->failToOpen = true;
    const auto registry = rtype::test::plugins::makeFakeAudioRegistry(state);
    const std::unique_ptr<rtype::plugins::audio::IAudioDevice> device =
        registry->createDevice(rtype::test::plugins::makeFakeAudioSettings());
    EXPECT_NE(dynamic_cast<rtype::plugins::audio::NullAudioDevice*>(device.get()), nullptr);
}

TEST(AudioRegistry, FallsBackToNullWhenTheFactoryReturnsNothing) {
    rtype::plugins::audio::AudioRegistry registry;
    registry.add("empty", [](const rtype::plugins::config::Settings& /*section*/,
                             const rtype::plugins::audio::AudioConfig& /*config*/) { return nullptr; });
    rtype::plugins::config::Settings settings;
    settings.set("audio.backend", "empty");
    const std::unique_ptr<rtype::plugins::audio::IAudioDevice> device = registry.createDevice(settings);
    EXPECT_NE(dynamic_cast<rtype::plugins::audio::NullAudioDevice*>(device.get()), nullptr);
}

TEST(AudioRegistry, ThrowsWhenARequiredDeviceCannotOpen) {
    const auto state = std::make_shared<rtype::test::plugins::FakeAudioState>();
    state->failToOpen = true;
    const auto registry = rtype::test::plugins::makeFakeAudioRegistry(state);
    rtype::plugins::config::Settings settings = rtype::test::plugins::makeFakeAudioSettings();
    settings.set("audio.required", true);
    EXPECT_THROW(static_cast<void>(registry->createDevice(settings)),
                 rtype::plugins::audio::exceptions::AudioBackendException);
}

TEST(AudioRegistry, NeverHidesAConfigMistakeBehindTheFallback) {
    const auto state = std::make_shared<rtype::test::plugins::FakeAudioState>();
    const auto registry = rtype::test::plugins::makeFakeAudioRegistry(state);
    rtype::plugins::config::Settings settings = rtype::test::plugins::makeFakeAudioSettings();
    settings.set("fake.unknown", true);
    EXPECT_THROW(static_cast<void>(registry->createDevice(settings)),
                 rtype::plugins::config::exceptions::SettingsException);
}
