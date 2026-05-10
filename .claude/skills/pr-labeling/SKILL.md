---
description: "Semver and backport target labeling rules for Electron PRs. Load when creating or labeling a pull request on electron/electron."
---

# PR Labeling (write-access only)

When the user has write access to `electron/electron`, add these labels when creating PRs:

**Semver label** — one of:

- `semver/none` — build changes, refactors, CI, or anything with no end-user impact
- `semver/patch` — backwards-compatible bug fixes
- `semver/minor` — backwards-compatible new functionality
- `semver/major` — incompatible API changes

**Backport target labels** — add `target/{N}-x-y` for each supported release branch the change should land on. Default policy:

- **Bug fixes** — backport to all active release lines _except the oldest_
- **Security fixes** — backport to all active release lines _including the oldest_
- **Features (semver/minor) and breaking changes (semver/major)** — no backport labels; main-only by default

To find which release branches are active, check label colors — active `target/*` labels use color `#ad244f`, older/EOL ones use `#ededed`:

```bash
gh label list --repo electron/electron --search target/ --json name,color --jq '.[] | select(.color == "ad244f") | .name'
```
