---
name: open-pr
description: Open a draft pull request, or a stack of them, for the R-Type repository with the template, labels, assignee, milestone and project fields set. Use when asked to "open a PR", "create a pull request", "submit", "push this for review", or when a branch is ready for review.
---

# Open a pull request

Always a draft (`--draft`); never merge it, never push to `main`. Run the `verify` skill first and put its result in the
description. If the change touches what scripts can see and `manual/` is not in the diff, run `write-manual` first. Follow
this whole skill, the template, labels, assignee, milestone and project fields included, even for a one-line PR.

## Rules

- **One pull request per issue.** Changes that depend on each other are a stack, one branch and one PR per link, each
  based on the previous one, built with `gh stack` (v0.3+).
- **Title** in the commit norm: `<type>(<scope>): <lowercase summary>`.
- **Body** from `.github/pull_request_template.md`: Summary, Why, Changes, Stack, Test plan with the checklist, Closes.
  Write the assumptions you made in it. No AI attribution line.
- **`Closes #n`** for each issue the PR completes (one line per issue). In a stack, the PR that completes an issue closes
  it, not the others.
- **Stack position**: in the Stack section write "Part n of N" and the list `#a -> #b -> #c`.
- **Assignee**: the person running the command (`--assignee @me`); never a hard-coded login.
- **Labels**: the area labels of the issue (`ecs`, `luau`, `scripting`, `network`, `physics`, `Vulkan`...), plus
  `documentation`, `bug` or `enhancement` as fits. Priority and size are not labels.
- **Milestone**: the issue's, when it has one (`gh issue view <n> --json milestone`).
- **Project**: add it to project "R-Type" (number 8, owner `$OWNER`, the repository owner) with Status "In review".

## Single pull request

Get the owner once: `OWNER=$(gh repo view --json owner --jq .owner.login)`.

```sh
git push -u origin <branch>
gh pr create --draft --base main --title "<title>" --body-file <body.md> \
  --assignee @me --label ecs --label enhancement [--milestone "<name>"]
```

Fill the body in a file (the template is the skeleton) rather than inline.

## Stack

```sh
gh stack init <branch1> <branch2> <branch3>   # or: gh stack init, then gh stack add <next-branch> for each link
gh stack view                                 # check the order
gh stack submit                               # pushes the branches and creates the draft PRs, each on the previous
```

If `gh stack` is unavailable or fails, push each branch and create each PR with `--base <previous branch>` (the first on
`main`); retarget the bases by hand with `gh pr edit <n> --base <branch>`. Only the top pull request of a stack runs the
CI checks, on the whole stack; the others skip them, so say in each description what you ran locally. After `submit`,
edit each description to fill in "Part n of N" and the list of PR numbers (`gh pr edit <n> --body-file ...`).
`gh stack sync` and `gh stack rebase` keep the stack up to date after a change; never force-push a branch you did not
create.

## After creating each pull request

```sh
gh pr edit <n> --add-assignee @me --add-label documentation
gh project item-add 8 --owner $OWNER --url <pr url>
gh project item-edit 8 --owner $OWNER --url <pr url> --field Status --value "In review"
```

If this `gh` is older and `item-edit` does not take `--url`/`--field`, use ids: `gh project item-add ... --format json`
returns the item id; `gh project field-list 8 --owner $OWNER --format json` gives the field id and the option
id of "In review"; then `gh project item-edit --id <item> --project-id <project> --field-id <field>
--single-select-option-id <option>`. The project id comes from `gh project view 8 --owner $OWNER --format json`.

## Check

`gh pr view <n> --json isDraft,assignees,labels,milestone,projectItems,body` shows a draft, an assignee, the labels, the
milestone and the project item; fix what is missing.
