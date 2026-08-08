#!/usr/bin/env bash
#
# triage-prs.sh — list open PRs filtered by their CI status.
#
# Classifies each open (non-draft) PR by its GitHub checks:
#   green    all checks succeeded (SUCCESS/NEUTRAL/SKIPPED)
#   failing  at least one check failed (FAILURE/ERROR/CANCELLED/TIMED_OUT/...)
#   pending  checks exist but some are still running (PENDING/IN_PROGRESS/...)
#   none     no checks are running at all
#
# When scoped to a single named check (the default is the "GitHub Actions
# Completed" umbrella check), a SKIPPED result on that check is not treated as
# a pass: Electron skips the umbrella check when a required upstream job fails,
# so classification falls back to the full check set to find the real status.
#
# Draft PRs are skipped, as are PRs whose "PR Triage" board Status contains WIP
# (--no-triage skips the board lookup, so WIP PRs are included when it is set).
#
# Requires the `gh` CLI, authenticated via `gh auth login`, and `jq`.
#
# Usage:
#   .claude/skills/triage-prs/triage-prs.sh                      # green PRs (default)
#   .claude/skills/triage-prs/triage-prs.sh --status failing
#   .claude/skills/triage-prs/triage-prs.sh --status none
#   .claude/skills/triage-prs/triage-prs.sh --status all --limit 100
#   .claude/skills/triage-prs/triage-prs.sh --check "GitHub Actions Completed"
#   .claude/skills/triage-prs/triage-prs.sh --check "faraday/cage" --status pending
#
# Options:
#   --status <s>          Which PRs to show: green | failing | pending | none | all
#                         (default: green)
#   --check <name>        Only consider the named check instead of all checks.
#                         (default: "GitHub Actions Completed"). Pass an empty
#                         string to evaluate every check on the PR.
#   --triage / --no-triage
#                         Show (default) or hide the Status field from the
#                         "PR Triage" project next to each PR. Requires the
#                         gh token to have the 'read:project' scope
#                         (run: gh auth refresh -s read:project).
#   --triage-project <n>  Name of the project board to read Status from
#                         (default: "PR Triage").
#   --repo <owner/name>   Target repository (default: electron/electron)
#   --limit <n>           Max number of open PRs to scan (default: 200)
#   -h, --help            Show this help and exit

set -euo pipefail

REPO="${REPO:-electron/electron}"
LIMIT="${LIMIT:-200}"
CHECK_NAME="${CHECK_NAME:-GitHub Actions Completed}"
STATUS="${STATUS:-green}"
TRIAGE="${TRIAGE:-1}"
TRIAGE_PROJECT="${TRIAGE_PROJECT:-PR Triage}"

# Print the header comment block (everything between the shebang and the first
# non-comment line) as help text. Deriving the range this way keeps --help in
# sync when the header grows, unlike a hard-coded line range.
usage() {
  awk 'NR == 1 { next } /^#/ { sub(/^# ?/, ""); print; next } { exit }' "$0"
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    --status)         STATUS="$2";         shift 2 ;;
    --check)          CHECK_NAME="$2";     shift 2 ;;
    --triage)         TRIAGE=1;            shift   ;;
    --no-triage)      TRIAGE=0;            shift   ;;
    --triage-project) TRIAGE_PROJECT="$2"; shift 2 ;;
    --repo)           REPO="$2";           shift 2 ;;
    --limit)          LIMIT="$2";          shift 2 ;;
    -h|--help) usage; exit 0 ;;
    *) echo "Unknown argument: $1" >&2; usage; exit 1 ;;
  esac
done

case "$STATUS" in
  green|failing|pending|none|all) ;;
  *) echo "error: --status must be one of: green, failing, pending, none, all" >&2; exit 1 ;;
esac

if ! command -v gh >/dev/null 2>&1; then
  echo "error: the GitHub CLI (gh) is required but was not found on PATH." >&2
  exit 1
fi
if ! command -v jq >/dev/null 2>&1; then
  echo "error: jq is required but was not found on PATH." >&2
  exit 1
fi

OWNER="${REPO%%/*}"
NAME="${REPO##*/}"

# Reading a ProjectV2 board's fields needs the 'read:project' token scope. If it
# is missing, disable the triage column rather than failing every PR.
if [[ "$TRIAGE" == "1" ]]; then
  # Reading a project's `title` is what requires read:project, so probe that.
  probe=$(gh api graphql -f owner="$OWNER" -f name="$NAME" -f query='
    query($owner:String!,$name:String!){
      repository(owner:$owner,name:$name){ projectsV2(first:1){ nodes{ title } } }
    }' 2>&1 || true)
  if grep -q 'INSUFFICIENT_SCOPES' <<<"$probe"; then
    echo "note: gh token lacks the 'read:project' scope, so the \"${TRIAGE_PROJECT}\" Status" >&2
    echo "      column is disabled. Enable it with: gh auth refresh -s read:project" >&2
    echo >&2
    TRIAGE=0
  fi
fi

# Fetch the Status field value from the triage project for one PR (may be empty).
triage_status() {
  gh api graphql \
    -f owner="$OWNER" -f name="$NAME" -F number="$1" \
    -f query='
      query($owner:String!,$name:String!,$number:Int!){
        repository(owner:$owner,name:$name){
          pullRequest(number:$number){
            projectItems(first:20){
              nodes{
                project{ title }
                fieldValueByName(name:"Status"){
                  ... on ProjectV2ItemFieldSingleSelectValue { name }
                }
              }
            }
          }
        }
      }' 2>/dev/null |
    jq -r --arg p "$TRIAGE_PROJECT" '
      [ .data.repository.pullRequest.projectItems.nodes[]?
        | select(.project.title == $p)
        | (.fieldValueByName.name // empty) ] | first // ""
    ' 2>/dev/null || printf ''
}

# Icon for each classification.
icon_for() {
  case "$1" in
    green)   printf '✅' ;;
    failing) printf '❌' ;;
    pending) printf '🟡' ;;
    none)    printf '⚪' ;;
  esac
}

scope_desc="all checks"
[[ -n "$CHECK_NAME" ]] && scope_desc="the \"${CHECK_NAME}\" check"

if [[ "$STATUS" == "all" ]]; then
  echo "Scanning up to ${LIMIT} open PRs in ${REPO}, classifying by ${scope_desc}..."
else
  echo "Scanning up to ${LIMIT} open PRs in ${REPO} for ${STATUS} PRs (by ${scope_desc})..."
fi
echo

found=0

# List open, non-draft PR numbers, then inspect each PR's status check rollup.
while read -r number; do
  [[ -z "$number" ]] && continue

  json=$(gh pr view "$number" --repo "$REPO" \
    --json statusCheckRollup,title,url,labels 2>/dev/null) || continue

  # Classify the PR. statusCheckRollup mixes GitHub Actions CheckRun entries
  # (status/conclusion) and legacy StatusContext entries (state). When --check
  # is set, only that named check is considered; otherwise every check is.
  # gh's --jq does not accept jq's --arg, so pipe raw JSON to jq for $name.
  class=$(echo "$json" | jq -r --arg name "$CHECK_NAME" '
    def state_of:
      # CheckRun entries expose .status (QUEUED/IN_PROGRESS/COMPLETED) and,
      # only once COMPLETED, a .conclusion. An in-progress CheckRun reports
      # .conclusion as "" (empty string, not null), so key off .status
      # first. StatusContext entries only expose .state.
      if (.status != null and .status != "COMPLETED") then .status
      elif ((.conclusion // "") != "") then .conclusion
      elif ((.state // "") != "") then .state
      else "PENDING"
      end;
    # Reduce a list of resolved states to a single classification, or null for
    # an empty list.
    def classify($s):
      if ($s | length) == 0 then null
      elif any($s[]; . == "FAILURE" or . == "ERROR" or . == "CANCELLED"
                     or . == "TIMED_OUT" or . == "ACTION_REQUIRED"
                     or . == "STARTUP_FAILURE" or . == "STALE") then "failing"
      elif any($s[]; . == "PENDING" or . == "EXPECTED" or . == "IN_PROGRESS"
                     or . == "QUEUED" or . == "REQUESTED"
                     or . == "WAITING") then "pending"
      else "green"
      end;
    (.statusCheckRollup // []) as $all
    | ($all | map(state_of)) as $allStates
    | ( if $name == "" then $all
        else [ $all[] | select((.name // .context) == $name) ]
        end ) as $matched
    | ($matched | map(state_of)) as $s
    | if $name == "" then
        # Evaluating every check on the PR.
        ( classify($s) // "none" )
      elif ($s | length) == 0 then
        # No matching check. Only "none" when the PR has zero checks at all;
        # if other checks exist, the named (umbrella) check simply has not
        # been posted yet, so CI is still pending.
        ( if ($all | length) == 0 then "none" else "pending" end )
      elif any($s[]; . == "SKIPPED") then
        # The named (umbrella) check was SKIPPED. In Electron the "GitHub
        # Actions Completed" umbrella check is skipped when a required upstream
        # job failed, so it never ran to a real conclusion. A SKIPPED umbrella
        # is therefore NOT a pass; fall back to the full check set to determine
        # the true status.
        ( classify($allStates) // "green" )
      else
        ( classify($s) // "green" )
      end
  ')

  if [[ "$STATUS" != "all" && "$class" != "$STATUS" ]]; then
    continue
  fi

  IFS=$'\t' read -r title url < <(echo "$json" | jq -r '[.title, .url] | @tsv')

  # Flag PRs that carry the "new-pr" label so they stand out as needing a first
  # look. The label name includes a trailing emoji ("new-pr 🌱"), so match by
  # prefix. jq exits non-zero-safely; default to empty if the label is absent.
  new_pr_suffix=""
  if echo "$json" | jq -e '.labels[]? | select(.name | startswith("new-pr"))' >/dev/null 2>&1; then
    new_pr_suffix="  [new-pr]"
  fi

  # Optionally look up the PR's Status on the triage project board.
  triage=""
  if [[ "$TRIAGE" == "1" ]]; then
    triage=$(triage_status "$number")
  fi

  # Exclude PRs marked WIP on the triage board; these are not ready for review.
  if [[ "$triage" == *WIP* ]]; then
    continue
  fi

  triage_suffix=""
  [[ -n "$triage" ]] && triage_suffix="  [${TRIAGE_PROJECT}: ${triage}]"

  # Emit an OSC 8 hyperlink so terminals that support it render a clickable
  # link on the PR reference; the raw URL is printed too for plain terminals.
  printf '%s \e]8;;%s\e\\#%s\e]8;;\e\\  %s%s%s\n     %s\n' \
    "$(icon_for "$class")" "$url" "$number" "$title" "$new_pr_suffix" "$triage_suffix" "$url"
  found=$((found + 1))
done < <(gh pr list --repo "$REPO" --state open --limit "$LIMIT" \
  --json number,isDraft --jq '.[] | select(.isDraft | not) | .number')

echo
if [[ "$STATUS" == "all" ]]; then
  echo "Done. ${found} non-draft PR(s) scanned and classified."
else
  echo "Done. ${found} ${STATUS} PR(s) (by ${scope_desc})."
fi
