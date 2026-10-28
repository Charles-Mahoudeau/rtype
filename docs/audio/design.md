# Audio Design

This document describes the audio of the R-Type engine as planned: an ECS plugin, `AudioPlugin`, that plays sounds
through a swappable audio backend chosen by the configuration, the same way `BackendPlugin` starts the platform and
the renderer a config names. The first backend is [miniaudio](https://miniaud.io), chosen in
[`../benchmarks/audio.md`](../benchmarks/audio.md); a silent `"null"` backend ships with the plugin, and FMOD can be
added later without touching gameplay code.

> Status: **the plugin is implemented and tested (steps 1 to 4 of the [plan](#11-implementation-plan)), with a
> console example, [`examples/plugins/sound/AudioBasics`](../../examples/plugins/sound/AudioBasics/src/main.cpp); the
> miniaudio backend and the engine wiring are next.** Snippets omit the Epitech header and most `[[nodiscard]]`/
> `noexcept` for brevity. Names are relative to `rtype::plugins::audio` unless qualified. Decisions and what is still
> open are in [Open questions](#12-open-questions).

## Contents

1. [Scope](#1-scope)
2. [Why a registry](#2-why-a-registry)
3. [Where it lives](#3-where-it-lives)
4. [Configuration](#4-configuration)
5. [The backend contract: `IAudioDevice`](#5-the-backend-contract-iaudiodevice)
6. [The registry](#6-the-registry)
7. [Sounds as assets: `AudioAssets`](#7-sounds-as-assets-audioassets)
8. [The plugin: resources, components, events, systems](#8-the-plugin-resources-components-events-systems)
9. [Backends](#9-backends)
10. [Threading, errors and the server](#10-threading-errors-and-the-server)
11. [Implementation plan](#11-implementation-plan)
12. [Open questions](#12-open-questions)

## 1. Scope

- **What a 2D shoot'em up needs:** one music track with crossfades, a few dozen short sound effects at once, volume
  per bus (music, effects), and left/right panning from where the sound happens on screen.
- **Gameplay never sees a backend.** It loads sounds by path into `AssetId`s, sends events and adds components, as it
  does for sprites. No miniaudio or FMOD type leaves its backend module.
- **Client only.** The server simulates and replicates; each client decides what to play from what it receives.
- **Not in scope yet:** 3D positional audio (HRTF, Doppler, occlusion), effects (reverb, filters), FMOD Studio events
  and banks. The contract leaves room for them (see [Toward richer backends](#toward-richer-backends)).

## 2. Why a registry

The benchmark left one question open: miniaudio as a private package of the `plugins` target (as Jolt for physics), or
behind a contract implemented by its own target (as rendering). This design takes the second option:

- **`plugins` keeps depending on nothing but the ECS, glm and Jolt.** The audio library is linked by the executable
  only, which is also where GLFW and Vulkan are chosen.
- **A second backend exists from day one:** `"null"`, which plays nothing. Tests, the CI runners (no sound card) and a
  client started with sound off use it, so the contract is exercised by two implementations, not one.
- **FMOD stays possible.** Its SDK is closed and cannot be built in CI, so it can only ever be an optional target; a
  registry is what lets an optional target plug in without `#ifdef`s in the plugin.

The cost is small because the pattern already exists: `AudioRegistry` mirrors `BackendRegistry`, and
`audio-miniaudio` mirrors `platform-glfw`.

**Why not add audio to `BackendRegistry`?** `createBackend()` starts a platform and a renderer that are tied to each
other (the renderer draws in the platform's window). Audio needs neither: it has its own registry, its own plugin and
its own config section, so a game can have a window without sound, or sound without a window.

## 3. Where it lives

```
src/rtype/plugins/audio/            namespace rtype::plugins::audio   (target plugins)
├── AudioPlugin.hpp/.cpp            the plugin: registers, inserts the resources, adds the systems
├── AudioRegistry.hpp/.cpp          backends by name; "null" is always registered
├── IAudioDevice.hpp                the contract every backend implements
├── AudioTypes.hpp                  SoundId, VoiceId, Bus, SoundDesc, PlayParams
├── AudioConfig.hpp/.cpp            the common `audio` section, typed and checked
├── AudioAssets.hpp/.cpp            the sounds by AssetId (resource)
├── AudioVolumes.hpp                master and bus volumes (resource)
├── AudioSource.hpp                 a sound attached to an entity (component)
├── AudioListener.hpp               where the player hears from (component)
├── AudioEvents.hpp                 PlaySound, PlayMusic, StopMusic
├── AudioResources.hpp              the "rtype.Audio" and "rtype.AudioSetup" resources
├── NullAudioDevice.hpp/.cpp        the silent backend
└── exceptions/AudioExceptions.hpp  AudioBackendException, AudioLoadException

src/rtype/audio/miniaudio/          namespace rtype::audio::miniaudio  (target audio-miniaudio)
├── MiniaudioDevice.hpp/.cpp        IAudioDevice on ma_engine, ma_sound, ma_sound_group
├── Registration.hpp/.cpp           registerAudio(AudioRegistry&) → "miniaudio"
├── exceptions/                     MiniaudioException
└── xmake.lua                       add_requires("miniaudio")

src/rtype/audio/fmod/               later, same shape, behind an xmake option (see FMOD)
```

- **`audio-miniaudio` is a static library** (`rtype-audio-miniaudio`), like `platform-glfw`: it links `plugins` for the
  contract and nothing else. Only its own files include `miniaudio.h`: `MiniaudioDevice.hpp` is internal, the
  executable includes `Registration.hpp` only.
- **Dependency rules** gain one line, with the same direction as the platforms and renderers:

  ```
  ecs  ←  plugins  ←  audio-*  ←  engine (executable)
  ```

  `plugins` never depends on an `audio-*` target, and an `audio-*` target never includes a platform, a renderer or
  ECS headers beyond what the contract needs.
- **The `null` backend lives in `plugins/audio/`**, since it needs no library: the registry registers it in its
  constructor, so it is there in every build, including tests.

## 4. Configuration

The plugin reads the same `core::config::Settings` as the backend plugin: a common `audio` section, plus one section per
backend named after it, handed to that backend's factory only (as `vulkan = { ... }` is to the Vulkan renderer).

```lua
return {
    platform = "glfw",
    renderer = "vulkan",
    window = { title = "R-Type", size = { 1280, 720 } },

    audio = {
        backend = "miniaudio",   -- a registered name: "miniaudio", "null", later "fmod"
        required = false,        -- if the device cannot open: false falls back to "null", true throws
        maxVoices = 64,          -- voices playing at once; the oldest effect is stolen beyond that
        volume = { master = 1.0, music = 0.7, effects = 1.0 },
        panDistance = 400,       -- world units from the listener at which a sound is fully left or right
    },
    miniaudio = {                -- only for the "miniaudio" backend
        sampleRate = 48000,      -- 0: the device's own rate
        channels = 2,            -- 0: the device's own channel count
        periodSizeInMilliseconds = 10, -- mixing block length: lower is less latency, more CPU; 0: the default
    },
}
```

- **The name is `audio.backend`, not a top-level `audio = "miniaudio"`,** because `audio` is already the common
  section: `Settings` is flat, and a key that is both a value and a section would be confusing.
- **`AudioConfig`** turns the `audio` section into a typed struct and checks it with `checkKeys()`, so a typo
  (`audio.maxVoice`) throws a `SettingsException` instead of being ignored, as `WindowConfig` does for `window`.
- **Each factory checks its own section**, as `registerPlatform()` does (`settings.checkKeys({...}, "miniaudio")`).
- **The volumes of the config are the starting values** of the `AudioVolumes` resource; a settings menu changes the
  resource, not the config.

## 5. The backend contract: `IAudioDevice`

The contract is the common ground of miniaudio's engine (`ma_engine`, `ma_sound`, `ma_sound_group`) and FMOD Core
(`System`, `Sound`, `Channel`, `ChannelGroup`): sounds, voices playing them, and buses mixing the voices. Nothing in it
is specific to one library.

```cpp
/// @brief A sound the backend has loaded, as TextureId is a texture the renderer has created.
struct SoundId {
    std::uint32_t value{0};  ///< 0 means "no sound".
};

/// @brief One playback of a sound. Stale once the voice ends: every call on a stale id does nothing.
struct VoiceId {
    std::uint32_t index{0};       ///< Slot in the backend's voice table.
    std::uint32_t generation{0};  ///< Bumped when the slot is reused, so an old id never controls a new voice.
};

/// @brief The buses voices are mixed into, each with its own volume, all under the master volume.
enum class Bus : std::uint8_t {
    kMusic,    ///< The music track.
    kEffects,  ///< Everything else.
};

/// @brief How a sound is loaded.
struct SoundDesc {
    bool stream{false};  ///< Decoded while it plays (music), instead of decoded once into memory (effects).
};

/// @brief How a voice starts. Every value can be changed while it plays.
struct PlayParams {
    Bus bus{Bus::kEffects};
    float volume{1.0F};  ///< Linear gain, 1 = unchanged.
    float pitch{1.0F};   ///< Playback speed, 1 = unchanged.
    float pan{0.0F};     ///< -1 = left, 0 = centre, 1 = right.
    bool loop{false};
    float fadeIn{0.0F};  ///< Seconds from silence to `volume`; 0 starts at once.
};

/// @brief An audio output: the contract every audio backend implements. Called from the main thread only.
class IAudioDevice {
  public:
    IAudioDevice() = default;
    virtual ~IAudioDevice() = default;
    IAudioDevice(const IAudioDevice&) = delete;
    IAudioDevice& operator=(const IAudioDevice&) = delete;
    IAudioDevice(IAudioDevice&&) = delete;
    IAudioDevice& operator=(IAudioDevice&&) = delete;

    /// @name Sounds
    /// @{
    /// @brief Loads a sound from an encoded file in memory (WAV, OGG, MP3, FLAC). The backend decodes it.
    /// @throws exceptions::AudioLoadException If the data cannot be decoded.
    virtual SoundId createSound(std::span<const std::byte> encoded, const SoundDesc& desc) = 0;
    /// @brief Loads a sound from a file; a streamed sound keeps reading it while it plays.
    virtual SoundId createSound(const std::filesystem::path& path, const SoundDesc& desc) = 0;
    /// @brief Stops every voice of the sound and frees it.
    virtual void destroySound(SoundId sound) = 0;
    /// @}

    /// @name Voices
    /// @{
    /// @return The new voice, or a stale id if no voice could be found (maxVoices reached, nothing to steal).
    virtual VoiceId play(SoundId sound, const PlayParams& params) = 0;
    /// @brief Stops the voice, fading out over @p fadeOut seconds.
    virtual void stop(VoiceId voice, float fadeOut = 0.0F) = 0;
    virtual void setPaused(VoiceId voice, bool paused) = 0;
    virtual void setVolume(VoiceId voice, float volume) = 0;
    virtual void setPitch(VoiceId voice, float pitch) = 0;
    virtual void setPan(VoiceId voice, float pan) = 0;
    /// @return False once the voice has ended or been stopped.
    [[nodiscard]] virtual bool isPlaying(VoiceId voice) const = 0;
    /// @}

    /// @name Mixing
    /// @{
    virtual void setMasterVolume(float volume) = 0;
    virtual void setBusVolume(Bus bus, float volume) = 0;
    /// @}

    /// @brief Called once per frame: reclaims the ended voices (miniaudio), or runs the library's update (FMOD's
    /// System::update()).
    virtual void update() = 0;
};
```

Design choices behind it:

- **The backend decodes, not the plugin.** Textures are decoded by `stb_image` in `plugins` and handed to the renderer
  as pixels; sounds are handed over encoded, because both miniaudio and FMOD decode and stream themselves, and
  streaming a music track cannot go through a fully decoded buffer.
- **Voices are generational handles.** A sound effect ends on its own on the audio thread; with a generation in the id,
  gameplay can keep a `VoiceId` as long as it likes and a call on an ended voice is a harmless no-op, with no callback
  crossing from the audio thread.
- **Panning is computed by the plugin**, from the listener's and the source's `GlobalTransform` (see
  [Systems](#systems)), and handed down as a plain `pan` and `volume`. Every backend then sounds the same, and the
  contract needs no positional API for a 2D game.
- **Buses are a fixed enum for now** (see [Open questions](#12-open-questions)). Both libraries map it directly:
  an `ma_sound_group` or an FMOD `ChannelGroup` per bus.

### Toward richer backends

What the contract leaves out is added as **optional interfaces**, discovered at runtime, as interop interfaces are for
the renderers (`docs/Backend.md`), so a backend that lacks them still fits:

- `ISpatialAudio`: `setListener(position, orientation)` and `setPosition(VoiceId, vec3)`, for 3D (miniaudio's and
  FMOD's own spatialization).
- `IAudioEvents`: FMOD Studio's events, banks and parameters. These are a different model (a sound designer's event
  rather than a file), so they get their own interface and their own components rather than being forced into
  `play()`.

## 6. The registry

```cpp
/// @brief Audio backends by name, so the configuration picks one instead of the code.
class AudioRegistry {
  public:
    /// @param section The backend's own section (`miniaudio`...), to check and read.
    /// @param config The common `audio` section, already typed.
    using Factory = std::function<std::unique_ptr<IAudioDevice>(const core::config::Settings& section,
                                                                const AudioConfig& config)>;

    /// @brief Registers "null".
    AudioRegistry();
    // Rule of five: non-copyable, non-movable, as BackendRegistry.

    /// @brief Registers a backend under a name, replacing any previous one with the same name.
    void add(std::string name, Factory factory);

    /// @return The registered names, sorted.
    [[nodiscard]] std::vector<std::string> getNames() const;

    /// @brief Creates the backend `audio.backend` names, with its section of @p settings.
    /// @throws exceptions::AudioBackendException If the name is missing or not registered, or if the device cannot
    /// open and `audio.required` is true. Falls back to "null", with a warning, if it cannot open and is not required.
    /// @throws exceptions::SettingsException If a setting is unknown or has the wrong type.
    [[nodiscard]] std::unique_ptr<IAudioDevice> createDevice(const core::config::Settings& settings) const;

  private:
    std::map<std::string, Factory, std::less<>> _factories;  ///< Factories by name.
};
```

The executable fills it, next to the backend registry:

```cpp
auto audio = std::make_unique<rtype::plugins::audio::AudioRegistry>();
rtype::audio::miniaudio::registerAudio(*audio);
app.addPlugin(rtype::plugins::audio::AudioPlugin{std::move(audio), settings});
```

## 7. Sounds as assets: `AudioAssets`

The resource `"rtype.AudioAssets"` works as `TextureAssets` does, so gameplay code reads the same for both:

```cpp
void spawnShip(rtype::ecs::Commands& commands, rtype::ecs::ResMut<rtype::plugins::audio::AudioAssets> sounds)
{
    const rtype::core::asset::AssetId engine = sounds->load("assets/sounds/engine.ogg");
    commands.spawn(rtype::plugins::audio::AudioSource{.sound = engine, .loop = true});
}
```

- `load(path, SoundDesc)` and `load(name, encodedBytes, SoundDesc)` queue the sound and return its `AssetId` at once
  (`AssetId::fromPath()`), without touching the backend: a server or a test gets the same ids.
- Each frame, `updateSounds(assets, device)` creates the queued sounds and destroys the unloaded ones. An entry is
  `kQueued`, `kLoaded` (with its `SoundId`) or `kFailed` (with the reason, printed): a missing sound does not stop the
  game, as a missing texture does not.
- Sounds known by `AssetId` are what gameplay, Luau and the network exchange; `SoundId` and `VoiceId` never leave the
  process.

## 8. The plugin: resources, components, events, systems

### Resources

| Resource | Kind | What it holds |
|---|---|---|
| `"rtype.Audio"` | main thread | `std::unique_ptr<IAudioDevice>`, created at Startup |
| `"rtype.AudioSetup"` | plain | the registry and the settings, handed to the Startup system (as `BackendSetup`) |
| `"rtype.AudioAssets"` | plain | the sounds by `AssetId` |
| `"rtype.AudioVolumes"` | plain | `master`, `music`, `effects`; starts from the config, applied when it changes |

### Components

```cpp
/// @brief A sound attached to an entity: starts when the component is added, stops when it or the entity goes.
struct AudioSource {
    AssetId sound;
    Bus bus{Bus::kEffects};
    float volume{1.0F};
    float pitch{1.0F};
    bool loop{false};
    bool spatial{true};  ///< Panned and attenuated from its GlobalTransform; false: centred.
};

/// @brief Where the player hears from: the entity with this and a GlobalTransform. None: sounds are not panned.
struct AudioListener {};
```

The plugin keeps the voice of each source in the `"rtype.Audio"` resource, by entity: each frame, the sources it no
longer finds (component removed, entity despawned) get their voice stopped. A map rather than an internal component,
because the ECS has no removal hook, and a component would go with its entity before the plugin could stop its voice.

### Events

```cpp
/// @brief A one-shot sound, fire and forget (a shot, an explosion).
struct PlaySound {
    AssetId sound;
    Bus bus{Bus::kEffects};
    float volume{1.0F};
    float pitch{1.0F};
    std::optional<glm::vec2> position;  ///< World position to pan from; none: centred.
};

/// @brief Replaces the music: the current track fades out while this one fades in, looping.
struct PlayMusic {
    AssetId sound;
    float crossfade{1.0F};  ///< Seconds.
};

/// @brief Fades the music out.
struct StopMusic {
    float fadeOut{1.0F};
};
```

Music is a single slot owned by the plugin, so gameplay never tracks a music voice itself. Its sound is best loaded
with `SoundDesc{.stream = true}`.

### Systems

All are client systems (`setSide(ecs::Side::kClient)`), as the backend plugin's are.

| Phase | Name | What it does |
|---|---|---|
| Startup | `rtype.audio.create` | `AudioRegistry::createDevice()` into `"rtype.Audio"`, applies `AudioVolumes` |
| PostUpdate | `rtype.audio.sounds` | `updateSounds()`: loads the queued sounds, destroys the unloaded ones |
| PostUpdate | `rtype.audio.play` | plays `PlaySound`, then `StopMusic`, then `PlayMusic` (a play sent with a stop wins); after `rtype.audio.sounds` |
| PostUpdate | `rtype.audio.sources` | starts new `AudioSource`s, stops removed ones, updates pan and volume; after `rtype.Transform.Propagate` |
| PostUpdate | `rtype.audio.update` | applies `AudioVolumes` if changed, then `IAudioDevice::update()`; last |

Running in PostUpdate, after the transform propagation, means a sound asked for during this frame's Update is heard in
this frame, from where its entity is now. An entity spawned this frame has no `GlobalTransform` yet (the
`TransformPlugin` adds it through commands applied at the end of PostUpdate), so the plugin reads its `Transform`
instead, for the source and for the listener.

**Volumes** are given to the device when it is created, then whenever `AudioVolumes` differs from what the device has:
the plugin keeps the last values it applied, so only a real change reaches the backend.

**Panning**, for a spatial source or a `PlaySound` with a position: `pan = clamp(dx / panDistance, -1, 1)` with `dx`
the source's x minus the listener's, in world units. Distance attenuation is left out for now: everything happens on
screen in a shoot'em up.

**A sound not loaded yet** when `PlaySound` arrives (asked for and played in the same frame, file still loading) is
played once loaded if that happens within the frame, else dropped with no error: a late shot sound is worse than a
missing one. An `AudioSource` waits for its sound instead, and starts once it is loaded.

## 9. Backends

### `null`

Hands out ids and tracks the voices' state (a looping voice plays until stopped, a one-shot voice ends at the next
`update()`), so the plugin's logic runs and is testable without any sound card. Always registered.

### `miniaudio`

- One `ma_engine` (device, mixer, resource manager) owned by `MiniaudioDevice`, one `ma_sound_group` per bus, one
  `ma_sound` per voice, created from the loaded sound's data source (`ma_sound_init_copy` for effects, a streamed
  `ma_sound` for music). The C objects are wrapped in RAII classes.
- `maxVoices` sizes the voice table; beyond it, the oldest effect voice is stolen, music never is.
- OGG Vorbis decodes through `stb_vorbis`, from the `stb` package already in the project.
- Settings: `sampleRate`, `channels`, `device` (see [Configuration](#4-configuration)).

### FMOD

- Its own target, `audio-fmod`, on FMOD Core (`FMOD::System`, `Sound`, `Channel`, `ChannelGroup` per bus), which maps
  the contract one to one.
- **Optional, off by default**: built only with an xmake option (for example `--Fmod=y`) and an SDK path, since the
  SDK is downloaded with an account and cannot be built in CI. The executable registers it only when the option is on.
- FMOD Studio (events, banks) comes through `IAudioEvents` if the game ever needs a sound designer's workflow.

## 10. Threading, errors and the server

- **Main thread only.** `"rtype.Audio"` is a main-thread resource and every `IAudioDevice` call comes from the main
  thread. The backends' audio threads stay inside them: no callback reaches the ECS, ended voices are found by
  polling in `update()`.
- **Errors.** `AudioBackendException` for a missing or unknown backend, or a device that cannot open while required;
  `AudioLoadException` for a sound that cannot be decoded (caught by `updateSounds()`, which marks it `kFailed`);
  config mistakes stay `SettingsException`. A device that fails to open (no output, a headless machine) falls back to
  `"null"` unless `audio.required` is true: no sound card must not mean no game.
- **The server.** The engine runs as server or client from one executable: the plugin's systems are client-side, and
  a headless server never adds the plugin, so it never opens a device. Sounds follow the replicated state: a client
  system turns "a laser appeared" or "a ship was hit" into `PlaySound`; nothing audio-related goes over the network.

## 11. Implementation plan

Each step builds and is tested on its own, in this order:

1. **Shared configuration and asset ids** (done): `Settings` and `SettingsException` moved out of `plugins/backend/`, to
   `core/config/` (first `core/config/`, then the `core` library, since `Settings` is not a plugin), `AssetId` to
   `core/asset/`, so `audio` does not depend on `backend`.
2. **The contract** (done): `AudioTypes`, `IAudioDevice`, `AudioConfig`, the exceptions, `NullAudioDevice`, `AudioRegistry`,
   with unit tests (`tests/plugins/`).
3. **`AudioAssets`** (done) and `updateSounds()`, tested against the null backend.
4. **`AudioPlugin`** (done): resources, components, events and systems, tested with the null backend and a recording test
   device.
5. **`audio-miniaudio`**: the target, `MiniaudioDevice`, `registerAudio()`; switch the `AudioBasics` example from its log device to
   `"miniaudio"`, or add a windowed one.
6. **The engine**: register miniaudio, add the plugin on the client, give the duel a music track and shot, hit and win
   sounds.
7. **Docs**: update `CLAUDE.md` (structure and dependency rules), `docs/ecs/design.md` (the `IAudio` line) and the
   benchmark's open question.

## 12. Open questions

Settled:

1. **`Settings`** moved, with `SettingsException`, to `core/config/` (`rtype::core::config`), in the `core` library,
   which depends on no other target. It was first in `core/config/`, but it is not a plugin.
   `BackendException` stays in `plugins/backend/exceptions/BackendExceptions.hpp`.
2. **`AssetId`** moved to `core/asset/` (`rtype::core::asset`).
3. **Buses** are a fixed enum (music, effects) for now.
4. **A device that cannot open** falls back to `"null"` with a warning, unless `audio.required` is true. A mistake in
   the config (`SettingsException`) is always thrown.

Still open:

5. **Distance attenuation.** Left out for a single-screen game; to add as `minDistance` / `maxDistance` if levels
   become larger than the screen.
