#!/usr/bin/env node

import { execSync } from 'node:child_process';
import * as fs from 'node:fs/promises';
import { dirname, resolve } from 'node:path';
import { fileURLToPath } from 'node:url';

import { compareVersions, getChromiumVersionFromDEPS } from './lib/utils.js';

const __dirname = dirname(fileURLToPath(import.meta.url));
const ELECTRON_DIR = resolve(__dirname, '..');

const CL_REGEX =
  /https:\/\/chromium-review\.googlesource\.com\/c\/(chromium\/src|devtools\/devtools-frontend|v8\/v8)\/\+\/(\d+)(#\S+)?/g;
const ROLLER_BRANCH_PATTERN = /^roller\/chromium\/(.+)$/;
const DEPS_BUMP_MSG_REGEX = /^chore: bump chromium in DEPS to (\S+)$/;

function getCurrentBranch() {
  // In CI, use `GITHUB_HEAD_REF` since we checkout the PR branch in detached HEAD state
  if (process.env.GITHUB_HEAD_REF) {
    return process.env.GITHUB_HEAD_REF;
  }

  try {
    return execSync('git rev-parse --abbrev-ref HEAD', {
      cwd: ELECTRON_DIR,
      encoding: 'utf8'
    }).trim();
  } catch {
    console.error('Could not determine current git branch');
    process.exit(1);
  }
}

function getCommitsSinceMergeBase(mergeBase) {
  try {
    const output = execSync(`git log --format=%H%n%B%n---COMMIT_END--- ${mergeBase}..HEAD`, {
      cwd: ELECTRON_DIR,
      encoding: 'utf8'
    });

    const commits = [];
    const parts = output.split('---COMMIT_END---');

    for (const part of parts) {
      const trimmed = part.trim();
      if (!trimmed) continue;

      const lines = trimmed.split('\n');
      const sha = lines[0];
      const message = lines.slice(1).join('\n');

      if (sha && message) {
        commits.push({ sha, message });
      }
    }

    return commits;
  } catch {
    return [];
  }
}

function getChangedFilesForCommit(sha) {
  try {
    return execSync(`git diff-tree --no-commit-id --name-only -r ${sha}`, {
      cwd: ELECTRON_DIR,
      encoding: 'utf8'
    })
      .trim()
      .split('\n')
      .filter(Boolean);
  } catch {
    return null;
  }
}

function getChromiumVersionForCommit(sha) {
  try {
    const depsContent = execSync(`git show ${sha}:DEPS`, {
      cwd: ELECTRON_DIR,
      encoding: 'utf8'
    });
    return getChromiumVersionFromDEPS(depsContent);
  } catch {
    return null;
  }
}

function getParentChromiumVersion(sha) {
  try {
    const depsContent = execSync(`git show ${sha}^:DEPS`, {
      cwd: ELECTRON_DIR,
      encoding: 'utf8'
    });
    return getChromiumVersionFromDEPS(depsContent);
  } catch {
    return null;
  }
}

async function fetchChromiumDashCommit(commitSha, repo) {
  const url = `https://chromiumdash.appspot.com/fetch_commit?commit=${commitSha}&repo=${repo}`;
  const response = await fetch(url);

  if (!response.ok) {
    return null;
  }

  try {
    return await response.json();
  } catch {
    return null;
  }
}

async function getGerritPatchDetails(clUrl) {
  const parsedUrl = new URL(clUrl);
  const match = /^\/c\/(.+?)\/\+\/(\d+)/.exec(parsedUrl.pathname);
  if (!match) {
    return null;
  }

  const [, repo, number] = match;
  const changeId = `${repo}~${number}`;
  const url = `https://chromium-review.googlesource.com/changes/${encodeURIComponent(changeId)}?o=CURRENT_REVISION`;
  const response = await fetch(url);

  if (!response.ok) {
    return null;
  }

  try {
    // Gerrit returns JSON with a magic prefix
    const text = await response.text();
    const jsonText = text.startsWith(")]}'") ? text.substring(4) : text;
    const data = JSON.parse(jsonText);

    // Get the current revision's commit SHA
    const currentRevision = data.current_revision;
    return {
      commitSha: currentRevision,
      project: data.project
    };
  } catch {
    return null;
  }
}

async function main() {
  const currentBranch = getCurrentBranch();

  // Check if we're on a roller/chromium/* branch
  const branchMatch = ROLLER_BRANCH_PATTERN.exec(currentBranch);
  if (!branchMatch) {
    console.log(`Not on a roller/chromium/* branch (current: ${currentBranch}), skipping lint.`);
    process.exit(0);
  }

  const targetBranch = branchMatch[1];
  console.log(`Linting roller branch: ${currentBranch} (target: ${targetBranch})`);

  // Get the merge base with the target branch
  let mergeBase;
  try {
    mergeBase = execSync(`git merge-base HEAD origin/${targetBranch}`, {
      cwd: ELECTRON_DIR,
      encoding: 'utf8'
    }).trim();
  } catch {
    console.error(`Could not determine merge base with origin/${targetBranch}`);
    process.exit(1);
  }

  // Get version range
  const baseVersion = getChromiumVersionForCommit(mergeBase);
  const depsContent = await fs.readFile(resolve(ELECTRON_DIR, 'DEPS'), 'utf8');
  const newVersion = getChromiumVersionFromDEPS(depsContent);

  if (!baseVersion || !newVersion) {
    console.error('Could not determine Chromium version range');
    console.error(`  Base version: ${baseVersion ?? 'unknown'}`);
    console.error(`  New version: ${newVersion ?? 'unknown'}`);
    process.exit(1);
  }

  console.log(`Chromium version range: ${baseVersion} -> ${newVersion}`);

  // Get all commits since merge base
  const commits = getCommitsSinceMergeBase(mergeBase);
  console.log(`Found ${commits.length} commits to check`);

  let hasErrors = false;
  const checkedCLs = new Map(); // Cache CL check results

  // Collect all commit titles for fixup validation
  const commitTitles = new Set(commits.map((c) => c.message.split('\n')[0]));

  for (const commit of commits) {
    const shortSha = commit.sha.substring(0, 7);
    const firstLine = commit.message.split('\n')[0];

    // Check for Skip-Lint trailer
    const skipLintMatch = /^Skip-Lint:\s*(.+)$/m.exec(commit.message);
    if (skipLintMatch) {
      console.log(`\nSkipping commit ${shortSha}: ${firstLine}`);
      console.log(`  ⏭️  Reason: ${skipLintMatch[1]}`);
      continue;
    }

    console.log(`\nChecking commit ${shortSha}: ${firstLine}`);

    // Validate fixup commits reference an existing commit title
    if (firstLine.startsWith('fixup! ')) {
      const targetTitle = firstLine.slice('fixup! '.length);
      if (!commitTitles.has(targetTitle)) {
        console.error(`  ❌ fixup! commit does not match any other commit title`);
        console.error(`     Target: "${targetTitle}"`);
        hasErrors = true;
      }
      continue;
    }

    // Validate Chromium version bump commits
    const parentVersion = getParentChromiumVersion(commit.sha);
    const commitVersion = getChromiumVersionForCommit(commit.sha);
    const depsVersionChanged = parentVersion && commitVersion && parentVersion !== commitVersion;
    const bumpMsgMatch = DEPS_BUMP_MSG_REGEX.exec(firstLine);

    if (depsVersionChanged) {
      const changedFiles = getChangedFilesForCommit(commit.sha);

      // Must be the only change in the commit
      if (!changedFiles || changedFiles.length !== 1 || changedFiles[0] !== 'DEPS') {
        const filesDesc = changedFiles ? changedFiles.join(', ') : 'unknown';
        console.error(`  ❌ Commit changes chromium_version in DEPS but also modifies other files: ${filesDesc}`);
        hasErrors = true;
      }

      // Commit message must match the expected pattern
      if (!bumpMsgMatch) {
        console.error(`  ❌ Commit changes chromium_version in DEPS but message does not match expected format`);
        console.error(`     Expected: "chore: bump chromium in DEPS to ${commitVersion}"`);
        console.error(`     Got:      "${firstLine}"`);
        hasErrors = true;
      } else if (bumpMsgMatch[1] !== commitVersion) {
        console.error(`  ❌ Chromium version in commit message does not match DEPS`);
        console.error(`     Message version: ${bumpMsgMatch[1]}`);
        console.error(`     DEPS version:    ${commitVersion}`);
        hasErrors = true;
      } else {
        console.log(`  ✅ DEPS bump commit to ${commitVersion} is valid`);
      }

      continue;
    }

    if (bumpMsgMatch && !depsVersionChanged) {
      console.error(`  ❌ Commit message says it bumps chromium in DEPS but chromium_version was not changed`);
      hasErrors = true;
      continue;
    }

    const cls = [...commit.message.matchAll(CL_REGEX)].map((match) => ({
      url: match[0],
      fullRepo: match[1],
      clNumber: match[2],
      fragment: match[3] ?? null
    }));

    if (cls.length === 0) {
      // No CLs in this commit, skip
      console.log(`  ⏭️  No CLs found`);
      continue;
    }

    for (const cl of cls) {
      // Skip CLs annotated with #nolint
      if (cl.fragment === '#nolint') {
        console.log(`  ⏭️  CL ${cl.clNumber}: skipped (#nolint)`);
        continue;
      }

      // Check cache first (use URL as key to distinguish same CL number across repos)
      if (checkedCLs.has(cl.url)) {
        const cached = checkedCLs.get(cl.url);
        if (cached.valid) {
          console.log(`  ✅ CL ${cl.clNumber}: (already validated)`);
        } else {
          console.error(`  ❌ CL ${cl.clNumber}: ${cached.error}`);
          hasErrors = true;
        }
        continue;
      }

      const repo = cl.fullRepo.split('/')[0];

      // Fetch Gerrit details to get commit SHA
      const gerritDetails = await getGerritPatchDetails(cl.url);

      if (!gerritDetails) {
        const error = 'Could not fetch CL details from Gerrit';
        checkedCLs.set(cl.url, { valid: false, error });
        console.error(`  ❌ CL ${cl.clNumber}: ${error}`);
        hasErrors = true;
        continue;
      }

      // Fetch from Chromium Dash to get the earliest version
      const dashDetails = await fetchChromiumDashCommit(gerritDetails.commitSha, repo);

      if (!dashDetails) {
        const error = 'Could not fetch commit details from Chromium Dash';
        checkedCLs.set(cl.url, { valid: false, error });
        console.error(`  ❌ CL ${cl.clNumber}: ${error}`);
        hasErrors = true;
        continue;
      }

      const clEarliestVersion = dashDetails.earliest;

      if (!clEarliestVersion) {
        const error = 'CL has no earliest version (may not be merged yet)';
        checkedCLs.set(cl.url, { valid: false, error });
        console.error(`  ❌ CL ${cl.clNumber}: ${error}`);
        hasErrors = true;
        continue;
      }

      // For non-Chromium CLs, we need to find the corresponding Chromium commit
      let chromiumVersion = clEarliestVersion;
      if (repo !== 'chromium' && dashDetails.relations) {
        const chromiumRelation = dashDetails.relations.find((rel) => rel.from_commit === gerritDetails.commitSha);
        if (chromiumRelation) {
          const chromiumDash = await fetchChromiumDashCommit(chromiumRelation.to_commit, 'chromium');
          if (chromiumDash?.earliest) {
            chromiumVersion = chromiumDash.earliest;
          }
        }
      }

      // CL should have landed after the base version and at or before the new version
      const isInRange =
        compareVersions(chromiumVersion, baseVersion) > 0 && compareVersions(chromiumVersion, newVersion) <= 0;

      if (!isInRange) {
        const error = `CL earliest version ${chromiumVersion} is outside roller range (${baseVersion} -> ${newVersion})`;
        checkedCLs.set(cl.url, { valid: false, error });
        console.error(`  ❌ CL ${cl.clNumber}: ${error}`);
        hasErrors = true;
        continue;
      }

      checkedCLs.set(cl.url, { valid: true });
      console.log(`  ✅ CL ${cl.clNumber}: version ${chromiumVersion} is within range`);
    }
  }

  console.log(hasErrors ? '\n❌ Lint failed' : '\n✅ Lint passed');
  if (hasErrors) {
    console.log('  NOTE: Add "Skip-Lint: <reason>" to a commit message to skip linting that commit.');
  }
  process.exit(hasErrors ? 1 : 0);
}

if ((await fs.realpath(process.argv[1])) === fileURLToPath(import.meta.url)) {
  main()
    .then(() => {
      process.exit(0);
    })
    .catch((err) => {
      console.error(`ERROR: ${err.message}`);
      process.exit(1);
    });
}
