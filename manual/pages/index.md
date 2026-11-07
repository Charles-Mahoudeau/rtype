# AYLO Engine Manual

The AYLO Engine runs games written entirely in Luau. You do not need to know anything about the engine's C++ code to
make one.

## A game is a folder

```
my_game/game/
  project.luau    the settings: window, renderer, audio, network, plugins, physics   (required)
  scripts/        your Luau files, loaded in sorted path order                        (optional)
  assets/         everything your game loads: images, sounds, fonts, maps             (required on a client)
```

The same generic program, `aylo-runtime`, runs every game: as the client by default, and as the server (no window, no
graphics) with `--server`. You point it at your folder with `--game`. Nothing in the engine knows about your game, and
your game never needs a program of its own.

## What you write

- **`project.luau`** chooses what the engine does: which plugins, which window, which network port.
  See [Project settings](project-settings.md).
- **Components** are the data of your game. See [Components](components.md).
- **Systems** are the rules that run on that data every tick or frame. See [Systems](systems.md).
- **Engine calls** let scripts use what plain data cannot express: loading an asset, playing a sound, asking the
  physics. See [Engine calls](engine-calls.md) and [Setup forms](setup-forms.md).
- **Capsules** are entity trees (an enemy, a boss and its turrets, a level) described in files under `assets/` and
  spawned from systems. See [Capsules](capsules.md), the [Capsule file format](capsule-format.md) and
  [Reading and writing capsule files](capsule-files.md).
- **The user interface** is built from tables: menus, HUDs, buttons, sliders. See [User interface](ui.md).
- **Particle effects** (sparks, smoke, trails) are tables too. See [Particles](particles.md).

## Client and server

A multiplayer game runs as one server and several clients. The server owns the gameplay (it is authoritative) and the
clients draw it and send their inputs. The same scripts load on both sides, and each system says where it runs. See
[Networking](networking.md).

## Where to start

1. [Getting started](getting-started.md) runs an example game.
2. [The R-Type Duel walkthrough](duel-walkthrough.md) reads a complete two-player game, file by file.
3. The reference pages answer "what can I write here".
4. [Shipping your game](shipping.md) explains how to download the engine and what to hand to your players.

!!! note "Planned"
    Parts of the engine are still being built. Pages mark what does not exist yet as *planned*: do not rely on it.
    Game states and a way for scripts to send their own network messages are planned.
