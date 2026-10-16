const { execFileSync } = require('node:child_process');
const path = require('node:path');

// Commits the result of re-exporting patches/ the way a roll is expected to be
// committed, so script/lint-roller-chromium-changes.mjs accepts the commits:
//
//   1. "chore: update patches" — patch files whose diff only touches hunk
//      headers, index lines and context lines. Mirrors build-tools
//      `e patches --commit-updates`, using the linter's definition of a
//      non-content change.
//   2. "chore: remove upstreamed patches" — deleted patch files plus their
//      lines in .patches, with a Ref: for each patch's Reviewed-on CL. With a
//      single Ref the title is "<CL>: <subject>" instead, as the linter expects.
//   3. "chore: update patches (content changes)" — anything else under
//      patches/. These changes need a CL reference that can't be derived
//      here, so this commit is left for a human (or the roll agent) to fix up.
//
// Run from the root of the electron checkout after the patches have been
// re-exported. Commits with the git identity from the environment.

const PATCHES_DIR = 'patches';
const NON_CONTENT_LINE = /^[-+]( |(--|\+\+|index|@@) )/;
const REVIEWED_ON = /^((?:> ?)*)Reviewed-on: (https:\/\/\S+\/\+\/(\d+))\s*$/;

function git(args, options = {}) {
  return execFileSync('git', args, { encoding: 'utf8', maxBuffer: 100 * 1024 * 1024, ...options });
}

function listChanged(filter) {
  return git(['diff', '-z', '--name-only', `--diff-filter=${filter}`, '--', PATCHES_DIR])
    .split('\0')
    .filter(Boolean);
}

function hasStagedChanges() {
  try {
    git(['diff', '--cached', '--quiet']);
    return false;
  } catch {
    return true;
  }
}

function commit(message) {
  git(['commit', '--no-verify', '--quiet', '-F', '-'], { input: message });
  console.log(`Committed: ${message.split('\n')[0]}`);
}

function isNonContentChange(filename) {
  const diff = git(['diff', '--', filename]);
  return [...diff.matchAll(/^[-+].*$/gm)].every((line) => NON_CONTENT_LINE.test(line[0]));
}

// Returns the CL a patch was cherry-picked from, preferring the patch's own
// Reviewed-on trailer over the ones quoted from the original change.
function getUpstreamCL(patchContent) {
  const lines = patchContent.split('\n');
  const end = lines.findIndex((line) => line.startsWith('diff --git '));
  const header = end === -1 ? lines : lines.slice(0, end);

  let best = null;
  for (const line of header) {
    const match = REVIEWED_ON.exec(line);
    if (!match) continue;
    const depth = match[1].replace(/ /g, '').length;
    if (best === null || depth <= best.depth) {
      best = { depth, url: match[2], number: match[3] };
    }
  }
  return best;
}

// The linter requires a commit with a single CL to be titled with that CL's
// subject as Gerrit has it, which the patch's Subject header doesn't preserve.
async function getGerritSubject(cl) {
  try {
    if (new URL(cl.url).hostname !== 'chromium-review.googlesource.com') return null;
    const response = await fetch(`https://chromium-review.googlesource.com/changes/${cl.number}`);
    if (!response.ok) return null;
    const text = await response.text();
    return JSON.parse(text.startsWith(")]}'") ? text.substring(4) : text).subject ?? null;
  } catch {
    return null;
  }
}

async function main() {
  // 1. Non-content changes, as `e patches --commit-updates` would commit them
  const trivial = listChanged('d').filter(isNonContentChange);
  if (trivial.length > 0) {
    git(['add', '--', ...trivial]);
    commit('chore: update patches');
  }

  // 2. Patches that no longer apply because they were upstreamed
  const removed = listChanged('D').filter((filename) => filename.endsWith('.patch'));
  if (removed.length > 0) {
    const refs = new Map();
    const removedByList = new Map();
    for (const filename of removed) {
      const cl = getUpstreamCL(git(['show', `HEAD:${filename}`]));
      if (cl) {
        refs.set(cl.url, cl);
      } else {
        console.warn(`No Reviewed-on CL found for removed patch ${filename}`);
      }

      const list = path.posix.join(path.posix.dirname(filename), '.patches');
      if (!removedByList.has(list)) removedByList.set(list, new Set());
      removedByList.get(list).add(path.posix.basename(filename));
    }

    git(['rm', '--cached', '--quiet', '--', ...removed]);
    // Stage only the removed patches' lines in each .patches file, so any
    // other change to the list ends up in the content-changes commit
    for (const [list, names] of removedByList) {
      let original;
      try {
        original = git(['show', `HEAD:${list}`]);
      } catch {
        continue;
      }
      const updated = original
        .split('\n')
        .filter((line) => !names.has(line.trim()))
        .join('\n');
      if (updated === original) continue;
      const blob = git(['hash-object', '-w', '--stdin'], { input: updated }).trim();
      git(['update-index', '--cacheinfo', `100644,${blob},${list}`]);
    }

    const cls = [...refs.values()];
    const subject = cls.length === 1 ? await getGerritSubject(cls[0]) : null;
    const title = subject ? `${cls[0].number}: ${subject}` : 'chore: remove upstreamed patches';
    const body = cls.map((cl) => `Ref: ${cl.url}`).join('\n');
    commit(body ? `${title}\n\n${body}\n` : `${title}\n`);
  }

  // 3. Everything else, which needs a CL reference added by hand
  git(['add', '--', PATCHES_DIR]);
  if (hasStagedChanges()) {
    commit('chore: update patches (content changes)');
  }
}

if (require.main === module) {
  main().catch((err) => {
    console.error(err);
    process.exit(1);
  });
}
