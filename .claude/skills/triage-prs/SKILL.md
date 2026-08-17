---
name: triage-prs
description: Triage open electron/electron pull requests by CI check status — lists PRs that are green/passing, failing, still pending, or have no checks at all, annotated with their Status on the "PR Triage" project board and with the new-pr label. Can scope to a single named check, e.g. PRs whose "faraday/cage" check is pending. Use when asked to triage PRs or do PR triage, list green/passing or mergeable PRs, find PRs with failing or pending CI, find PRs with no checks running, check faraday/cage status, or show each PR's triage board status. Also triggers on the bare words "faraday" or "failing prs".
---

# Triage PRs By CI Status

Lists open pull requests filtered by their GitHub checks. Backed by the bundled
script [`triage-prs.sh`](./triage-prs.sh), which lives alongside this file.

## Shortcuts

- **`faraday`** — when the user says just "faraday" (or "faraday prs",
  "pending faraday", etc.), run:

  ```bash
  .claude/skills/triage-prs/triage-prs.sh --check "faraday/cage" --status pending
  ```

  This lists open PRs whose `faraday/cage` check is still pending.

- **`failing`** — when the user says just "failing prs" (or "failing ci" in the
  context of PR triage), run:

  ```bash
  .claude/skills/triage-prs/triage-prs.sh --status failing
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

Each listed PR is also annotated with its **Status** on the **"PR Triage"**
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

## Runtime

The script makes up to two `gh` calls per PR, sequentially, so a full scan at
the default `--limit 200` takes **several minutes** and consumes a few hundred
API requests. Run it with an explicit long timeout (10 minutes is safe), or
lower `--limit` for a quicker partial scan. Do not assume it has hung.

## Usage

Run the script from the repo root:

```bash
# Green PRs (default), 200 most recent open PRs in electron/electron
.claude/skills/triage-prs/triage-prs.sh

# Filter by a different status
.claude/skills/triage-prs/triage-prs.sh --status failing
.claude/skills/triage-prs/triage-prs.sh --status none
.claude/skills/triage-prs/triage-prs.sh --status pending

# Classify every scanned PR and show its status
.claude/skills/triage-prs/triage-prs.sh --status all

# Scan fewer PRs for a faster result
.claude/skills/triage-prs/triage-prs.sh --status failing --limit 50

# Target another repo, or change the check scope
.claude/skills/triage-prs/triage-prs.sh --repo electron/forge --limit 50
.claude/skills/triage-prs/triage-prs.sh --check ""          # evaluate all checks
.claude/skills/triage-prs/triage-prs.sh --check "macos-x64 / build / build"

# Scope to a single named check, e.g. find PRs whose faraday/cage is pending
.claude/skills/triage-prs/triage-prs.sh --check "faraday/cage" --status pending
.claude/skills/triage-prs/triage-prs.sh --check "faraday/cage" --status failing

# Triage board Status column
.claude/skills/triage-prs/triage-prs.sh --no-triage                  # hide the Status column
.claude/skills/triage-prs/triage-prs.sh --triage-project "PR Triage" # read a different board
```

Environment variables `REPO`, `LIMIT`, `CHECK_NAME`, `STATUS`, `TRIAGE`, and
`TRIAGE_PROJECT` are honored as defaults and can be overridden by the
corresponding flags.

Output is one entry per PR — an icon, a clickable
[OSC 8 hyperlink](https://gist.github.com/egmontkob/eb114294efdcfddb1b38f3714a0de4d6)
on the PR number, the title, and any markers — with the raw URL on the next
line:

```text
✅ #52533  feat: support `restrictOwnAudio` constraint  [PR Triage: Needs Review]
     https://github.com/electron/electron/pull/52533
```

## Reporting results

When summarizing to the user, render each PR as a Markdown link so it is
clickable in chat, e.g. `[#52533](https://github.com/electron/electron/pull/52533) — feat: support restrictOwnAudio constraint`.
The script's output includes the PR URL on the second line for each entry, so
use that URL directly.

When a PR carries the `new-pr` label (the script appends a `[new-pr]` marker),
indicate it in the summary with the plain text `[new-pr]` rather than an emoji.

## Interpreting results

- Two categories of PR are **silently excluded** from every run, and neither is
  reported in the totals. Say so when summarizing, so the output is not read as
  the complete set of open PRs:
  - **Draft** PRs.
  - PRs whose **"PR Triage" board Status contains `WIP`** — these are treated as
    not ready for review. Note that `--no-triage` skips the board lookup
    entirely, so WIP PRs _are_ included when it is passed.
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
- A `SKIPPED` umbrella check is not treated as a pass: Electron skips
  "GitHub Actions Completed" when a required upstream job fails, so the script
  falls back to the full check set to find the real status.
