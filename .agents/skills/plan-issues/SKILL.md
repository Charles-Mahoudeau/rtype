---
name: plan-issues
description: Turn a plan document or a feature into a GitHub epic with child issues, sub-issue links, blocked-by dependencies and project fields, with the gh CLI, in the R-Type repository. Use when asked to "create the issues for this plan", "file the tasks", "create an epic", "break this down into issues", "link issues", "set blocked by", "add to the project", or to file a single issue or bug.
---

# Plan and file issues

Uses `gh` only (works in Claude Code and Codex). The repository is the current one: `OWNER=$(gh repo view --json owner --jq .owner.login)`, and `$R` is
`repos/$(gh repo view --json nameWithOwner --jq .nameWithOwner)`. Issue types may not exist on a user-owned repository: do not set one.

The project "R-Type" (user project number 8) holds the metadata and is the single source of truth: **Status**
(Backlog | Ready | In progress | In review | Done), **Priority** (P0 | P1 | P2 | P3), **Size** (XS | S | M | L | XL),
**Sprint**. Do not use priority or weight labels; area labels (`ecs`, `luau`, `scripting`, `network`, `physics`,
`Vulkan`, `documentation`, `bug`, `enhancement`, `epic`) stay.

## 0. Check for duplicates

```sh
gh issue list --state all --search "<keywords>" --limit 30
gh issue list --label epic --state open
```

If an issue already covers it, update it instead of filing a new one.

## 1. Write the plan first

When there are several tasks, write them in a plan document in `docs/` before filing anything (see
`docs/network/implementation.md` and `docs/scripting/plan.md`): the task list, a task id per task, and the dependency
graph between them. Task ids are `<PREFIX>-NN` (`NET-06`, `SCR-03`), used only when a plan document lists the tasks.
The issues are made from the document, not the other way round. For a single issue or a bug, skip to step 3.

## 2. Create the epic, then the children in dependency order

Titles: `[Area] <summary>` for a single issue, `[Area] <ID>: <summary>` for a task of a plan (`[Scripting] SCR-03:
Component views`), where Area is one of ECS, Luau, Scripting, Network, Physics, Vulkan, Audio, Engine, Docs... The epic
carries the `epic` label and a checklist of the tasks. Create the children in **topological order** (a blocker before
what it blocks), so every blocker has its number when you need it.

Bodies follow the issue template of `.github/ISSUE_TEMPLATE/` (Summary, Context, Usage, Tasks, Acceptance criteria,
Technical notes, References), with the task id first when there is one. Put the body in a file:

```sh
gh issue create --title "[Network] NET-06: Reliable sender" --body-file body.md \
  --label enhancement --label network [--milestone "<name>"] [--assignee @me]
```

`gh issue create` prints the issue URL; keep the number.

## 3. Link the children to the epic and to their blockers

The REST API takes the issue's **database id**, not its number:

```sh
gh api $R/issues/<n> --jq .id          # database id of issue n
```

Sub-issue of an epic (in order):

```sh
gh api $R/issues/<epic>/sub_issues -X POST -F sub_issue_id=<child database id>
```

Blocked-by (one call per blocker):

```sh
gh api $R/issues/<n>/dependencies/blocked_by -X POST -F issue_id=<blocker database id>
```

Never write "Blocked by #x" as text only: a text mention links nothing. You may repeat it in the body for readers.

## 4. Project fields

```sh
gh project item-add 8 --owner $OWNER --url <issue url>
gh project item-edit 8 --owner $OWNER --url <issue url> --field Status   --value Backlog
gh project item-edit 8 --owner $OWNER --url <issue url> --field Priority --value P1
gh project item-edit 8 --owner $OWNER --url <issue url> --field Size     --value M
```

Status is Backlog by default, Ready when nothing blocks it. Size: XS under an hour, S a few hours, M a day, L a few
days, XL a week or more (split an XL). If `gh` is older and refuses `--url`/`--field`, use ids as in the `open-pr` skill.

## 5. Read everything back

Verify the graph matches the plan before you report:

```sh
gh api $R/issues/<epic>/sub_issues --jq '[.[].number]'                       # every child, in order
gh api $R/issues/<n>/dependencies/blocked_by --jq '[.[].number]'              # blockers of n
gh project item-list 8 --owner $OWNER --limit 200 --format json    # fields of every item
```

Compare the sub-issues and every blocked-by list with the plan's graph, and every item's Status, Priority and Size. Fix
differences, and list the issue numbers in your report. These GET endpoints can also be used to read an existing
graph (for example `gh api $R/issues/335/dependencies/blocked_by`).

## One issue, or a bug

Same title and body rules, one issue, area label plus `enhancement` or `bug`. Still set the parent epic if there is one,
the blockers, and the project fields. A bug states the steps, what happens, what was expected, the platform and the
commit.
