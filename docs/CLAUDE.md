# Electron Documentation Guide

## API History Migration

Guide: `docs/development/api-history-migration-guide.md`
Style rules: `docs/development/style-guide.md` (see "API History" section)
Schema: `docs/api-history.schema.json`
Lint: `npm run lint:api-history`

### Format

Place YAML history block directly after the Markdown header, before parameters:

````md
### `module.method(args)`

<!--
```YAML history
added:
  - pr-url: https://github.com/electron/electron/pull/XXXXX
```
-->

* `arg` type - Description.
````

### Finding when an API was added

- `git log --all --reverse --oneline -S "methodName" -- docs/api/file.md` — find first commit adding a method name
- `git log --reverse -L :FunctionName:path/to/source.cc` — trace C++ implementation history
- `git log --grep="keyword" --oneline` — find merge commits referencing PRs
- `gh pr view <number> --repo electron/electron --json baseRefName` — verify PR targets main (not a backport)
- Always use the main-branch PR URL in history blocks, not backport PRs

### Cross-referencing breaking changes

- Search `docs/breaking-changes.md` for the API name to find deprecations/removals
- Use `git blame` on the breaking-changes entry to find the associated PR
- Add `breaking-changes-header` field using the heading ID from breaking-changes.md

### Placement rules

- Only add blocks to actual API entries (methods, events, properties with backtick signatures)
- Do NOT add blocks to section headers like `## Methods`, `### Instance Methods`, `## Events`
- Module-level blocks go after the `# moduleName` heading, before the module description quote
- For changes affecting multiple APIs, add a block under each affected top-level heading (see style guide "Change affecting multiple APIs")

### Key details

- `added` and `deprecated` arrays have `maxItems: 1`; `changes` can have multiple items
- `changes` entries require a `description` field; `added`/`deprecated` do not
- Wrap descriptions in double quotes to avoid YAML parsing issues with special characters
- Early Electron APIs (pre-2015) use merge-commit PRs (e.g., `Merge pull request #534`)
- Very early APIs (2013-2014, e.g., `ipcMain.on`, `ipcRenderer.send`) predate GitHub PRs — skip history blocks for these
- When multiple APIs were added in the same PR, they all reference the same PR URL
- Promisification PRs (e.g., #17355) count as `changes` entries with a description
  - These PRs are breaking changes and should have their notes as "This method now returns a Promise instead of using a callback function."
- APIs that were deprecated and then removed from docs don't need history blocks (the removal is in `breaking-changes.md`)
