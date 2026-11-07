---
name: verify
description: Verify a change in the R-Type repository against the definition of done before reporting it finished or opening a pull request. Use when you finished editing code, docs or scripts, when asked to "check", "verify", "test", "lint" or "make sure it builds", before committing or opening a pull request, and whenever you are about to say a task is done.
---

# Verify a change

The definition of done, as commands. Run it from the repository root, in this order, and report each step's real
outcome. CI runs the same checks on Linux, macOS and Windows.

## 1. Configure and build, with the tests

```sh
xmake f -y --Tests=y        # add --rtype_duel=y or --AllExamples=y if your change touches an example
xmake build -y
```

A change that touches no C++ (docs, Luau, `.github`) can skip the build, but say so.

## 2. Test

```sh
xmake test -v               # -v prints GoogleTest's output; without it you cannot see what failed
xmake test -v engine-tests/*    # one module: <module>-tests/*
```

New behavior has a test in `tests/<module>/`. A feature scripts can use also has a script test (see `tests/engine/`
for the engine calls and setup forms, `tests/scripting/` for the bridge).

## 3. Format and lint the changed files

```sh
xmake project -k compile_commands          # clang-tidy needs compile_commands.json at the root
.github/scripts/lint.sh origin/main        # base = the base of your branch (the stack's base in a stack)
```

`lint.sh` runs `clang-format` and `clang-tidy` on every C and C++ file changed between the base and `HEAD`, in full,
as CI does. To format only: `clang-format -i <files>`. To lint one file: `clang-tidy -p . <file>`.

## 4. Docs

The `docs/` page of the module you changed is updated; anything a script can use has its `manual/` page, written with
the `write-manual` skill. Run `git diff --stat <base>`: if the change touches an engine call, a component, an event, a
setting, a setup form or a plugin and `manual/` is not in the diff, stop and invoke `write-manual`. Check the
paths your docs, `AGENTS.md` and skills mention exist.

## 5. Report faithfully

- Say which steps you ran and the result of each: pass, fail, or not run, with the reason (tool missing, no GPU,
  platform).
- Never write "builds" or "tests pass" for a step you did not run. A failing step is reported with its first error, not
  hidden; a pre-existing failure is named as such.
- When part of the checks only CI can run (macOS, Windows, MSVC), say they are left to CI.
