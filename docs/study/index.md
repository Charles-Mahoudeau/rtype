# Technical and comparative study

Why the AYLO Engine is built with what it is built with. Each page compares the options we had, on criteria that come
from our own constraints, and says what we chose and what it costs us.

| Page | What it compares |
|---|---|
| [Technologies](technologies.md) | Language, build system and packages, graphics API, windowing, scripting, physics, audio, networking library, UI, editor, tests, documentation |
| [Algorithms and data structures](algorithms.md) | ECS storage, scheduling, events, replication, reliability over UDP, input quantization, the custom algorithms we wrote and why |
| [Storage](storage.md) | Saves, scene files, settings, assets: formats, persistence and reliability |
| [Security](security.md) | The vulnerabilities of each technology and of our own code, what we do about them, and how we keep watching |

## Our constraints

Every comparison is judged against the same list. They come from the subject and from the [architecture
rules](https://github.com/Charles-Mahoudeau/rtype/blob/main/AGENTS.md#architecture-rules).

| # | Constraint | Where it comes from |
|---|---|---|
| C1 | **C++**, built from source with a build system generator and a package manager, nothing installed on the system but compilers, SDKs and low-level platform libraries | Subject |
| C2 | **Linux and Windows** (and macOS, which we also use), with Clang, Apple Clang and MSVC | Subject, rule 10 |
| C3 | **A game engine, not a game**: features are plugins, a game is data and Luau scripts, a second game must run on the same engine | Subject (track 1), rules 1 to 3 |
| C4 | **Networked, authoritative server**, binary protocol over UDP, a server that never waits for a client and never crashes on bad bytes | Subject, rule 6 |
| C5 | **Headless server**: the server links no window, GPU or audio library | Rule 4 |
| C6 | **No global state**, one world per room, one thread per room | Rule 8 |
| C7 | **A third-party library stays private** to the one module that wraps it, so it can be replaced | Rule 7 |
| C8 | **Free and open source**, buildable in CI on three platforms | Practical |

## How we decided

- Libraries were compared on these criteria and, when the difference could be measured, measured: see the benchmarks
  in [`../benchmarks/`](../benchmarks/) (physics, audio, scene file format).
- Each subsystem's design document has a "Why we need it / How others do it / Our choice" section; this study gathers
  their conclusions and links to them for the details.
- A choice behind a contract (renderer, platform, audio) can be changed without touching the plugins or the games:
  that is the point of [the contracts](../Backend.md).
