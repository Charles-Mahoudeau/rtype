---
name: commit-split
description: Split pending changes into atomic Conventional Commits in the R-Type repository and commit them. Use when committing, when asked to "commit", "split the commits", "write the commit messages", or before pushing a branch with several changes.
---

# Atomic commits

Commit only when the task or the run authorizes it (autonomous runs do; an interactive request to commit does). Never
add a `Co-Authored-By` trailer or any AI attribution line to a commit.

## Message norm

`<type>(<optional scope>): <lowercase summary>`, with the types used in this repository: `feat`, `fix`, `chore`,
`style`, `refactor`, `docs`, `test`. The scope is the affected area (`vulkan`, `window`, `ci`, `skills`, `ecs`...).
Example: `feat(vulkan): implement Window class for GLFW window management`.

Every commit has a body, separated from the subject by a blank line, saying what was done and why (`git commit -m
"<subject>" -m "<body>"`).

## Branches

Named `<type>/<area>/<name>` in kebab-case (`feat/vulkan/window`, `feat/skills/github-issues`) and merged into `main`
through pull requests. Never commit on `main`: branch first.

## Method

1. List every changed or new file (`git status`, `git diff --stat`).
2. For each one, ask: does this work and make sense on its own? A file that does (a helper, a utility, a type, an
   exception, a build-file change) gets its own commit, made before the commit of the code that uses it, so every commit
   builds and the history reads bottom-up (dependency first, consumer after). Example: a new `MapHelper.hpp` hash helper
   and the `Table` class that uses it are two commits: `feat(luau): add transparent string hash helper`, then
   `feat(luau): add Table class`. "Added in the same session" or "same feature" is not a reason to group files.
3. Group files only when they cannot exist separately (a class's `.hpp` and `.cpp`, or a change and the call sites it
   forces to update).
4. Tests go in their own `test(...)` commit, separate from the `feat` or `fix` they cover. Never mix unrelated concerns
   (a feature, a refactor, a formatting pass) in one commit.
5. If the pending changes cover several concerns, plan several commits. With more than one, write down the list
   (subject and files of each) before running them, then run them in order.
6. Stage explicitly by path (`git add <files>`), never `git add -A` or `git add .`, so each commit contains exactly the
   files planned for it. Check with `git diff --cached --stat` before committing.
7. Before the first commit, make sure the files are formatted (`clang-format -i`). Do not commit generated files
   (`compile_commands.json`, build output, the executables copied to the root).

## After

`git log --oneline <base>..HEAD` and read it as the reviewer will: each line tells one thing, in dependency order. Then
the `verify` skill, and `open-pr` when the work is ready for review.
