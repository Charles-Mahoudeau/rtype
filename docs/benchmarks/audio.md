# C/C++ audio library benchmark (October 2026)

**Context:** our engine is C++23, built with `xmake`, around our own ECS (`rtype::ecs`). Audio was removed from the
backend contracts until a library is chosen (see [`ecs/design.md`](../ecs/design.md)): it will come back as its own
plugin, with a resource of sounds by `AssetId` and sounds triggered by events. The game is a 2D shoot'em up: a music
track, a few dozen short sound effects at once (shots, explosions, hits), left/right panning at most.

What our architecture imposes on the choice:

- **Plugins own their state.** The ECS keeps no global mutable state: the audio device and mixer must live in a
  resource of the `App`, not behind a hidden global context.
- **Headless server.** The `server` executable never plays sound and must not link the audio library.
- **Our window is GLFW.** A library that brings its own windowing layer (SDL, SFML) would duplicate it.
- **Open source, no account.** The library must build from source in CI on the three platforms, with a license that
  does not constrain how we link it.
- **Three platforms, three compilers.** Linux, macOS and Windows; Clang, Apple Clang and MSVC must build it.

> This is a comparison on criteria, not a timed benchmark. At our load (one music stream and a few dozen voices),
> the CPU cost of every library below is negligible; what separates them is features, license and integration.

---

## 1. Overview

| Library | Kind | Language / API | License | Status (Oct. 2026) | Maturity | `xmake-repo` package |
|---|---|---|---|---|---|---|
| **miniaudio** | Playback + decoding + mixing engine | C (single library, no dependencies) | Public domain / MIT-0 | Very active | ★★★★★ | `miniaudio` (0.11.25) |
| **OpenAL Soft** | 3D positional playback | C (OpenAL 1.1 API, global context) | LGPL-2.0 | Active | ★★★★★ | `openal-soft` (1.24.3) |
| **SDL3 audio + SDL3_mixer** | Audio streams + mixer | C | zlib | Active | ★★★★☆ | `libsdl3` (3.4.x) |
| **SFML Audio** | Playback + decoding | C++ | zlib | Active (SFML 3 audio is built on miniaudio) | ★★★★☆ | `sfml` (2.5.1 only) |
| **SoLoud** | Playback + decoding + mixing engine | C++ | zlib | Last release in 2020 | ★★★☆☆ | `soloud` (2020.02.07) |
| **PortAudio / RtAudio** | Raw device I/O | C / C++ | MIT | Active | ★★★★☆ | `portaudio`, `rtaudio` |
| **FMOD / Wwise** | Full middleware + authoring tool | C / C++ | Proprietary (free under revenue limits) | Industry standard | ★★★★★ | none |
| **Steam Audio** | Spatialization only (HRTF, occlusion) | C | Apache-2.0 | Active | ★★★★☆ | `steam-audio` |

The package column comes from our local `xmake-repo` index (October 2026); "none" means a custom package rule in our
`xmake.lua`.

---

## 2. Comparison on our criteria

Scores out of 5, applied to **our** project.

| Criterion | miniaudio | OpenAL Soft | SDL3 + mixer | SFML Audio | SoLoud |
|---|---|---|---|---|---|
| Features for a 2D game (mixing, groups, fades, streaming) | **5** | 2 (sources and buffers only) | 4 | 4 | **5** |
| Built-in decoding (WAV, OGG, MP3, FLAC) | **5** (OGG through `stb_vorbis`) | 1 (needs a decoder) | 4 (with SDL3_mixer) | 5 | 5 |
| Fits our ECS (state in a resource, no globals) | **5** (`ma_engine` is a plain object) | 2 (global current context) | 3 (SDL global init) | 4 | 4 |
| No extra platform layer next to GLFW | **5** | **5** | 1 (brings SDL) | 2 (brings SFML) | 5 |
| License | **5** (public domain) | 3 (LGPL: dynamic linking) | 5 | 5 | 5 |
| `xmake` integration | **5** (package, no dependencies) | 4 (package, plus a decoder package) | 4 | 2 (old 2.5.1 package) | 4 |
| Documentation / samples | 4 | 4 | 4 | **5** | 3 |
| Longevity | **5** | 5 | 5 | 4 | 2 |
| **Total /40** | **39** | 26 | 30 | 31 | 33 |

PortAudio, RtAudio, FMOD, Wwise and Steam Audio are left out of the scores: they are either too low-level, closed or
not a playback library (see [Library details](#3-library-details)).

---

## 3. Library details

### miniaudio

- **Pros:** one library with no dependencies that talks to the system directly (CoreAudio, WASAPI, PulseAudio, ALSA,
  JACK). Its high-level engine (`ma_engine`, `ma_sound`, `ma_sound_group`) covers everything a 2D game needs:
  mixing, volume groups (music, sound effects), fades, pitch, panning, looping, and asynchronous loading and
  streaming through its resource manager. It decodes WAV, FLAC and MP3 itself, and OGG Vorbis through `stb_vorbis`,
  which our `stb` package already ships.
- **3D later:** `ma_sound` also has positional audio (listener, attenuation, Doppler, cones), enough to follow a
  move to 3D without switching libraries.
- **ECS fit:** every state lives in objects we own (`ma_engine`, `ma_sound`), so a resource of the `App` can hold
  the engine and a resource of sounds by `AssetId` can hold the decoded data, as `TextureAssets` does for textures.
- **Cons:** a C API (we wrap it in RAII classes), and its callbacks run on the audio thread, so anything the plugin
  shares with them must be lock-free or posted to the main thread.

### OpenAL Soft

- **Pros:** the reference open implementation of OpenAL, with good 3D positional audio and HRTF.
- **Cons:** the API only plays buffers through sources: no decoding, no streaming helper, no groups, no fades. We
  would add a decoder (`libsndfile` or `dr_libs` + `stb_vorbis`) and rewrite the mixing features miniaudio already
  has. OpenAL keeps a **global current context**, against our "state lives in the world" rule. The LGPL license
  forces dynamic linking.

### SDL3 audio + SDL3_mixer

- **Pros:** solid, well maintained, and SDL3_mixer gives a game-oriented mixer.
- **Cons:** we already use GLFW for the window and input. Pulling in SDL for audio alone means two platform layers,
  two global initializations and a heavier build on three platforms.

### SFML Audio

- **Pros:** the nicest C++ API of the list (`sf::Sound`, `sf::Music`).
- **Cons:** it comes with the rest of SFML, and our `xmake-repo` only packages SFML 2.5.1, which predates SFML 3.
  Since SFML 3 audio is itself built on miniaudio, we can use miniaudio directly.

### SoLoud

- **Pros:** a very game-oriented API (voices, groups, filters, 3D) with built-in decoders.
- **Cons:** its last release dates from 2020. Building on it means maintaining it ourselves on three compilers.

### PortAudio, RtAudio, FMOD, Wwise, Steam Audio

- **PortAudio / RtAudio:** raw device I/O for pro audio. No decoding, no mixer: we would write the whole engine.
- **FMOD / Wwise:** the industry standard, with an authoring tool for sound designers. But they are closed binaries
  downloaded with an account, not buildable from source in CI, and their licenses add terms we do not need.
- **Steam Audio:** spatialization only (HRTF, occlusion, reverb). It plugs into a playback library; it does not
  replace one. Worth a look only if the game becomes 3D with a strong focus on sound.

---

## 4. Fitting it into our engine

### Plugin

An `AudioPlugin` in `plugins/audio/` (`rtype::plugins::audio`), declaring its own components, resources and events
like the other plugins:

- a main-thread resource holding the `ma_engine` (the audio device and mixer), with music and sound-effect groups;
- an `AudioAssets` resource: the sounds by `AssetId`, queued by `load()` and decoded on load, as `TextureAssets`
  does for textures;
- events for gameplay (`PlaySound`, `PlayMusic`, `StopMusic`), read by an `Update` system that starts the sounds;
- later, an `AudioSource` component for sounds attached to an entity (panning from its `GlobalTransform`).

Gameplay never sees miniaudio: it emits events with `AssetId`s, as it does for sprites.

### Server

The server does not add the plugin. As long as miniaudio is only included by `.cpp` files of `plugins/audio/`, the
server links nothing audio-related it does not use.

### Build

- `add_requires("miniaudio")`, added to `plugins` as a **private** package, as Jolt is: no public header includes
  `miniaudio.h`.
- **Open question: plugin or backend.** Two options to decide as a team before writing the code:
  1. **A plugin with a private package** (as physics with Jolt): simplest, one target, miniaudio stays an
     implementation detail of `plugins/audio/`.
  2. **A backend** (as rendering): an `IAudio` contract in `plugins/backend/`, implemented by an `audio-miniaudio`
     target registered in the `BackendRegistry` by the executable. More decoupled and swappable, but more code for
     a library we have no reason to swap.

---

## 5. Conclusion

### Recommendation: **miniaudio**

It is the best fit for **our** project:

1. **Everything a 2D game needs in one library:** mixing, groups, fades, streaming and decoding, with no
   dependencies.
2. **Fits our ECS:** no global state, so the engine and the sounds live in resources of the `App`.
3. **No extra platform layer** next to GLFW, and a public-domain license with no linking constraint.
4. **Ready for 3D** with its positional audio, if the game moves to 3D.

**Two points to watch:** callbacks run on the audio thread (nothing shared with them without care), and where the
integration lives (see [Build](#build)).

### OpenAL Soft: not recommended

Good for 3D positional audio, but for us it means adding a decoder, rewriting the mixer features and working around
its global context, all under the LGPL.

### To avoid

SDL3 and SFML (a second platform layer next to GLFW), SoLoud (unmaintained), PortAudio and RtAudio (too low-level),
FMOD and Wwise (closed, not buildable in CI).
