---
name: triage-prs
description: Find open electron/electron pull requests by their CI status — PRs with green (passing) checks, failing checks, still-pending checks, or no checks running at all, optionally annotated with their Status on the "PR Triage" project board. Also scopes to a single named check (e.g. find PRs whose "faraday/cage" check is pending or failing). Use when asked to list PRs with passing/green CI, find mergeable PRs, find PRs with failing CI, find PRs with no checks running, find PRs with a pending/failing faraday/cage (or other named) check, classify open PRs by their check status, or show each PR's triage/board status. When the request is simply "faraday", run the faraday shortcut described below; when it is simply "failing", run the failing shortcut described below.
---

# Find PRs By CI Status

Lists open (non-draft) pull requests filtered by their GitHub checks. Backed by
the bundled script [`script/triage-prs.sh`](../../../script/triage-prs.sh).

## Shortcuts

- **`faraday`** — when the user says just "faraday" (or "faraday prs",
  "pending faraday", etc.), run:

  ```bash
  ./script/triage-prs.sh --check "faraday/cage" --status pending
  ```

  This lists open PRs whose `faraday/cage` check is still pending.

- **`failing`** — when the user says just "failing" (or "failing prs",
  "failing ci", etc.), run:

  ```bash
  ./script/triage-prs.sh --status failing
  ```

  This lists open PRs whose checks are failing.

By default, evaluation is scoped to the **"GitHub Actions Completed"** check —
Electron's umbrella check that reflects the overall GitHub Actions run. Pass
`--check ""` to evaluate every check on the PR instead, or `--check "<name>"`
for a different single check.

Each PR is classified as one of:

| Status    | Icon | Meaning |
|-----------|------|---------|
| `green`   | ✅   | All checks succeeded (`SUCCESS`/`NEUTRAL`/`SKIPPED`). |
| `failing` | ❌   | At least one check failed (`FAILURE`/`ERROR`/`CANCELLED`/`TIMED_OUT`/…). |
| `pending` | 🟡   | Checks exist but some are still running (`PENDING`/`IN_PROGRESS`/…). |
| `none`    | ⚪   | No checks are running at all. |

Each listed PR can also be annotated with its **Status** on the **"PR Triage"**
GitHub project board (e.g. `[PR Triage: Needs Review]`). This is shown by
default and requires the `gh` token to have the `read:project` scope; if the
scope is missing the script prints a one-time note and omits the column.

## Prerequisites

- The [`gh`](https://cli.github.com/) CLI, authenticated (`gh auth login`).
- `jq` on `PATH`.
- To show the triage board Status column, the token needs the `read:project`
  scope. Grant it with:

  ```bash
  gh auth refresh -s read:project
  ```

## Usage

Run the script from the repo root:

```bash
# Green PRs (default), 200 most recent open PRs in electron/electron
./script/triage-prs.sh

# Filter by a different status
./script/triage-prs.sh --status failing
./script/triage-prs.sh --status none
./script/triage-prs.sh --status pending

# Classify every scanned PR and show its status
./script/triage-prs.sh --status all

# Scan more PRs
./script/triage-prs.sh --status failing --limit 100

# Target another repo, or change the check scope
./script/triage-prs.sh --repo electron/electron --limit 100
./script/triage-prs.sh --check ""          # evaluate all checks
./script/triage-prs.sh --check "macos-x64 / build / build"

# Scope to a single named check, e.g. find PRs whose faraday/cage is pending
./script/triage-prs.sh --check "faraday/cage" --status pending
./script/triage-prs.sh --check "faraday/cage" --status failing

# Triage board Status column
./script/triage-prs.sh --no-triage                  # hide the Status column
./script/triage-prs.sh --triage-project "PR Triage" # read a different board
```

Environment variables `REPO`, `LIMIT`, `CHECK_NAME`, `STATUS`, `TRIAGE`, and
`TRIAGE_PROJECT` are honored as defaults and can be overridden by the
corresponding flags.

## How it works

1. `gh pr list --state open` enumerates open PR numbers (capped by `--limit`).
   Draft PRs are skipped.
2. For each PR, `gh pr view --json statusCheckRollup,title,url` returns the
   checks on the PR head commit plus its title and URL (one API call per PR).
3. The rollup mixes two shapes, both handled:
   - **GitHub Actions** `CheckRun` entries expose `.name`, `.status`, and
     `.conclusion`.
   - **Legacy** `StatusContext` entries expose `.context` + `.state`.
   By default only the **"GitHub Actions Completed"** check is considered;
   `--check ""` widens evaluation to every check, and `--check <name>` scopes to
   a different single check. The collected states are reduced to one
   classification (`none` → `failing` → `pending` → `green`, in priority order).
4. PRs matching the requested `--status` (or all of them, for `--status all`)
   are printed as a clickable link followed by the title, with the raw PR URL on
   the next line:

   ```text
   ✅ #52533  feat: support `restrictOwnAudio` constraint  [PR Triage: Needs Review]
        https://github.com/electron/electron/pull/52533
   ```

   The `#<number>` is emitted as an [OSC 8 hyperlink](https://gist.github.com/egmontkob/eb114294efdcfddb1b38f3714a0de4d6)
   pointing at the PR URL, so terminals that support it render it clickable; the
   raw URL is always printed too for plain terminals and for easy copying. The
   icon reflects the PR's classification (useful with `--status all`).
5. Unless `--no-triage` is passed, a second call
   (`gh api graphql … projectItems … fieldValueByName(name:"Status")`) fetches
   the PR's Status on the **"PR Triage"** board and appends it as
   `[PR Triage: <status>]`. PRs not on the board show no suffix. If the token
   lacks the `read:project` scope, the column is skipped with a one-time note.

## Reporting results

When summarizing to the user, render each PR as a Markdown link so it is
clickable in chat, e.g. `[#52533](https://github.com/electron/electron/pull/52533) — feat: support restrictOwnAudio constraint`.
The script's output includes the PR URL on the second line for each entry, so
use that URL directly.

When a PR carries the `new-pr` label (the script appends a `[new-pr]` marker),
indicate it in the summary with the plain text `[new-pr]` rather than an emoji.

## Interpreting results

- With a status filter (the default is `green`), only PRs whose classification
  matches are listed; all others are omitted.
- `none` means the PR genuinely has no checks at all (nothing has been
  triggered yet), which is different from `pending` (checks exist but haven't
  finished).
- Because evaluation defaults to the **"GitHub Actions Completed"** check, a PR
  reads as `pending` while its other checks run but that umbrella check has not
  been posted yet, then flips to `green`/`failing` once the umbrella check
  reports. A PR is only `none` when it has zero checks whatsoever. Use
  `--check ""` to judge by all checks instead.

## Adjusting the criteria

- Use `--status green|failing|pending|none|all` to choose which PRs to show.
- Use `--check "<check name>"` to evaluate a different single check, or
  `--check ""` to evaluate every check on the PR instead of the default
  `"GitHub Actions Completed"`.
- Use `--no-triage` to omit the board Status column, or `--triage-project`
  `"<name>"` to read Status from a project other than `"PR Triage"`.
- The classification buckets are defined by the `jq` expression in the script;
  edit the `any($s[]; …)` clauses to change which conclusions count as failing
  or pending.
