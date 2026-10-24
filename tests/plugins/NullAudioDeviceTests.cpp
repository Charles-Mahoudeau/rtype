/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** NullAudioDeviceTests
*/

#include <gtest/gtest.h>

#include <cstddef>
#include <filesystem>
#include <span>

#include "plugins/audio/AudioTypes.hpp"
#include "plugins/audio/NullAudioDevice.hpp"

namespace {
/// @return A sound created from a file path in @p device.
rtype::plugins::audio::SoundId loadSound(rtype::plugins::audio::NullAudioDevice& device) {
    return device.createSound(std::filesystem::path{"assets/shot.wav"}, {});
}
}  // namespace

TEST(NullAudioDevice, LoadsEverySound) {
    rtype::plugins::audio::NullAudioDevice device;
    const rtype::plugins::audio::SoundId fromFile = loadSound(device);
    const rtype::plugins::audio::SoundId fromMemory = device.createSound(std::span<const std::byte>{}, {});
    EXPECT_NE(fromFile.value, 0U);
    EXPECT_NE(fromMemory.value, 0U);
    EXPECT_NE(fromFile, fromMemory);
}

TEST(NullAudioDevice, EndsAOneShotVoiceAtTheNextUpdate) {
    rtype::plugins::audio::NullAudioDevice device;
    const rtype::plugins::audio::VoiceId voice = device.play(loadSound(device), {});
    EXPECT_TRUE(device.isPlaying(voice));
    device.update();
    EXPECT_FALSE(device.isPlaying(voice));
}

TEST(NullAudioDevice, PlaysALoopingVoiceUntilStopped) {
    rtype::plugins::audio::NullAudioDevice device;
    const rtype::plugins::audio::VoiceId voice = device.play(loadSound(device), {.loop = true});
    device.update();
    device.update();
    EXPECT_TRUE(device.isPlaying(voice));
    device.stop(voice, 0.0F);
    EXPECT_FALSE(device.isPlaying(voice));
}

TEST(NullAudioDevice, KeepsAPausedVoice) {
    rtype::plugins::audio::NullAudioDevice device;
    const rtype::plugins::audio::VoiceId voice = device.play(loadSound(device), {});
    device.setPaused(voice, true);
    device.update();
    EXPECT_TRUE(device.isPlaying(voice));
    device.setPaused(voice, false);
    device.update();
    EXPECT_FALSE(device.isPlaying(voice));
}

TEST(NullAudioDevice, NeverLetsAStaleIdControlANewVoice) {
    rtype::plugins::audio::NullAudioDevice device;
    const rtype::plugins::audio::SoundId sound = loadSound(device);
    const rtype::plugins::audio::VoiceId old = device.play(sound, {});
    device.update();
    const rtype::plugins::audio::VoiceId reused = device.play(sound, {.loop = true});
    EXPECT_EQ(reused.index, old.index);
    EXPECT_NE(reused.generation, old.generation);
    device.stop(old, 0.0F);
    EXPECT_TRUE(device.isPlaying(reused));
}

TEST(NullAudioDevice, NeverHandsOutAVoiceThatADefaultIdNames) {
    rtype::plugins::audio::NullAudioDevice device;
    const rtype::plugins::audio::VoiceId voice = device.play(loadSound(device), {.loop = true});
    EXPECT_NE(voice, rtype::plugins::audio::VoiceId{});
    EXPECT_FALSE(device.isPlaying(rtype::plugins::audio::VoiceId{}));
}

TEST(NullAudioDevice, PlaysNothingBeyondItsVoiceCount) {
    rtype::plugins::audio::NullAudioDevice device{2};
    const rtype::plugins::audio::SoundId sound = loadSound(device);
    EXPECT_TRUE(device.isPlaying(device.play(sound, {.loop = true})));
    EXPECT_TRUE(device.isPlaying(device.play(sound, {.loop = true})));
    EXPECT_FALSE(device.isPlaying(device.play(sound, {.loop = true})));
}

TEST(NullAudioDevice, PlaysNothingForAnUnknownSound) {
    rtype::plugins::audio::NullAudioDevice device;
    EXPECT_FALSE(device.isPlaying(device.play(rtype::plugins::audio::SoundId{.value = 42}, {})));
}

TEST(NullAudioDevice, StopsTheVoicesOfADestroyedSound) {
    rtype::plugins::audio::NullAudioDevice device;
    const rtype::plugins::audio::SoundId sound = loadSound(device);
    const rtype::plugins::audio::VoiceId voice = device.play(sound, {.loop = true});
    device.destroySound(sound);
    EXPECT_FALSE(device.isPlaying(voice));
    EXPECT_FALSE(device.isPlaying(device.play(sound, {})));
}
