## Summary

<!-- What this pull request does, in one or two sentences. -->

## Why

<!-- The problem or the issue it answers. Assumptions you made go here too. -->

## Changes

<!-- The main changes, by module. -->

-

## Stack

<!-- "Part n of N" and the list of pull requests (#a -> #b -> #c), or "Not stacked". -->

Not stacked.

## Test plan

<!-- How you checked it; say what you did not run. In a stack only the top pull request runs the CI checks. -->

- [ ] `xmake build -y` (with `--Tests=y`) succeeds
- [ ] `xmake test -v` passes, new behavior has a test
- [ ] `clang-format` and `clang-tidy` are clean on the changed files (`.github/scripts/lint.sh <base>`)
- [ ] The docs are updated (`docs/`, and `manual/` for what scripts can use)
- [ ] A script can use any new engine feature (engine call, manual page, script test)

## Closes

<!-- One line per issue completed: "Closes #n". -->
