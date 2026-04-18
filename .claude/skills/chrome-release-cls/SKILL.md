---
name: chrome-release-cls
description: Given a Chrome Releases blog post URL (chromereleases.googleblog.com), extract every CVE/bug and find the underlying Gerrit CL that fixed it by searching the local Chromium checkout and sub-repos. Use when asked to map Chrome security release notes to fixing CLs, or to find which commits correspond to CVEs in a Chrome stable update.
---

# Chrome Release → Fixing CL Mapper

Maps every security fix in a Chrome Releases blog post to the Gerrit CL(s) that fixed it.

## Input

`$ARGUMENTS` — a `https://chromereleases.googleblog.com/...` URL. If empty, ask the user for one.

## Procedure

### 1. Extract CVE → bug ID pairs from the blog post

The blog HTML buries bug IDs inside `<a>` tags, so strip tags first. Run:

```bash
curl -sL "$URL" | python3 -c '
import sys, re, html
t = re.sub(r"<[^>]+>", " ", sys.stdin.read())
t = re.sub(r"\s+", " ", html.unescape(t))
seen = set()
for m in re.finditer(r"\[\s*(\d{6,})\s*\]\s*(Critical|High|Medium|Low)\s*(CVE-\d{4}-\d+):\s*([^.]+?)\.", t):
    if m.group(3) in seen: continue
    seen.add(m.group(3))
    print(f"{m.group(3)}|{m.group(1)}|{m.group(2)}|{m.group(4).strip()}")
' > /tmp/cve_bugs.txt
cat /tmp/cve_bugs.txt
```

If this yields nothing, the page may have changed format — fall back to `grep -oE 'CVE-[0-9]{4}-[0-9]+'` and `grep -oE 'crbug\.com/[0-9]+'` and pair them by order.

### 2. Find the fixing CL for each bug

Search git history in the Chromium checkout and relevant sub-repos for commits whose `Bug:` or `Fixed:` footer references the bug ID, then extract the `Reviewed-on:` Gerrit URL.

Repo selection by component keyword:
- ANGLE → `third_party/angle`
- Skia, Graphite → `third_party/skia`
- PDFium → `third_party/pdfium`
- Dawn → `third_party/dawn`
- V8, Turbofan, Maglev, Turboshaft → `v8`
- everything else → `.` (chromium/src)

Always also fall back to `.` if the hinted repo has no match.

```bash
cd /root/src/electron/src   # chromium root (parent of electron/)

lookup() {
  local bug="$1" repos="$2"
  for repo in $repos . v8 third_party/skia third_party/angle third_party/pdfium third_party/dawn; do
    local hits
    hits=$(git -C "$repo" log --all --since='6 months ago' -E \
           --grep="(Bug|Fixed):.*\\b${bug}\\b" --format='%H' 2>/dev/null | sort -u)
    [[ -z "$hits" ]] && continue
    while read -r h; do
      git -C "$repo" log -1 --format='%B' "$h" | grep '^Reviewed-on:' | sed 's/^/    /'
      echo "      ↳ $(git -C "$repo" log -1 --format='%s' "$h")"
    done <<<"$hits"
    return 0
  done
  echo "    (not found locally)"
}
```

Drive it from `/tmp/cve_bugs.txt`. Prefer the **non-`[M1xx]`-prefixed** commit subject as the canonical main CL; the `[M1xx]` ones are branch cherry-picks.

### 3. Handle misses

For any bug with no local hit:
- `git -C <repo> fetch origin` then re-search `--remotes` (fix may be newer than the checkout).
- Query Gerrit directly: `curl -s "https://chromium-review.googlesource.com/changes/?q=bug:${BUG}&n=10" | tail -n +2 | python3 -m json.tool` (also try `skia-review`, `pdfium-review`, `dawn-review`, `aomedia-review`).
- If still nothing and the bug was reported very recently (especially by "Google Threat Intelligence" or marked in-the-wild), the CL is likely still access-restricted — report it as such rather than guessing.

### 4. Special cases

- **libaom / libvpx / ffmpeg** components: the actual fix lands upstream; the chromium-side hit will be a `Roll src/third_party/...` commit. Report the roll CL and note the fix is upstream.
- Multiple `Reviewed-on:` lines in one commit body: cherry-picks keep the original line plus a new one. The **first** `Reviewed-on:` is the original CL.
- A bug may have multiple distinct fix CLs (fix + follow-up hardening) — list all of them.

### 5. Output

Produce a markdown table per severity level: `CVE | Bug | Component | Fix CL (main)`. Link bugs as `https://crbug.com/<id>`. Save raw output (including all branch merges) to `/tmp/cve_cls.txt` and mention the path.
