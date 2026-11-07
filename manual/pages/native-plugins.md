# Native plugins

Most games need nothing but Luau. When a game needs something scripts cannot do, such as heavy computation or a library
of its own, it can ship a **native plugin**: a C++ library in `<game>/plugins/` that the engine loads at start, like the
engine's own. Writing one is C++ work, so it is not covered here; this page is what a game developer needs to use one.

## Using a plugin

Name it in `project.luau`, as any plugin (see [Project settings](project-settings.md#plugins)). Its engine calls are then
available to scripts:

```luau
engine.settings.plugins = { "demo.Greeter" }
```

```luau
print(engine.call("demo.greet", { name = "pilot" }))
```

The plugin's section of the settings, if it has one, is read when it is built and checked like the others. A plugin
that is listed but missing stops the start with the names that exist.

## What the engine checks

- A library in `<game>/plugins/` cannot have the file name of an engine library: a game cannot replace part of the
  engine.
- A library is refused if it was built with another compiler or version than the engine.
- A plugin is native code and can do anything the player can, so a **release** build of the engine opens a library of
  `<game>/plugins/` only if it comes with a signature from a developer the project vouches for (`<library>.sig`, made
  with `rtype-sign`). A debug build checks nothing, as that is where plugins are written.

The folders, the entry points and the signatures in full are in `docs/Plugins.md` of the repository.
