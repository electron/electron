#!/usr/bin/env node

import { Octokit } from '@octokit/rest';
import { GitProcess } from 'dugite';
import { valid, compare, gte, lte } from 'semver';

import { basename } from 'node:path';
import { parseArgs } from 'node:util';

import { get, render } from './notes';
import { ELECTRON_DIR } from '../../lib/utils';
import { createGitHubTokenStrategy } from '../github-token';
import { ELECTRON_ORG, ELECTRON_REPO } from '../types';

const octokit = new Octokit({
  authStrategy: createGitHubTokenStrategy(ELECTRON_REPO)
});

const semverify = (version: string) => version.replace(/^origin\//, '').replace(/[xy]/g, '0').replace(/-/g, '.');

const runGit = async (args: string[]) => {
  console.info(`Running: git ${args.join(' ')}`);
  const response = await GitProcess.exec(args, ELECTRON_DIR);
  if (response.exitCode !== 0) {
    throw new Error(response.stderr.trim());
  }
  return response.stdout.trim();
};

const tagIsSupported = (tag: string) => !!tag && !tag.includes('nightly') && !tag.includes('unsupported');
const tagIsAlpha = (tag: string) => !!tag && tag.includes('alpha');
const tagIsBeta = (tag: string) => !!tag && tag.includes('beta');
const tagIsStable = (tag: string) => tagIsSupported(tag) && !tagIsBeta(tag) && !tagIsAlpha(tag);

const getTagsOf = async (point: string) => {
  try {
    const tags = await runGit(['tag', '--merged', point]);
    return tags.split('\n')
      .map(tag => tag.trim())
      .filter(tag => valid(tag))
      .sort(compare);
  } catch (err) {
    console.error(`Failed to fetch tags for point ${point}`);
    throw err;
  }
};

const getTagsOnBranch = async (point: string) => {
  const { data: { default_branch: defaultBranch } } = await octokit.repos.get({
    owner: ELECTRON_ORG,
    repo: ELECTRON_REPO
  });
  const mainTags = await getTagsOf(defaultBranch);
  if (point === defaultBranch) {
    return mainTags;
  }

  const mainTagsSet = new Set(mainTags);
  return (await getTagsOf(point)).filter(tag => !mainTagsSet.has(tag));
};

const getBranchOf = async (point: string) => {
  try {
    const branches = (await runGit(['branch', '-a', '--contains', point]))
      .split('\n')
      .map(branch => branch.trim())
      .filter(branch => !!branch);
    const current = branches.find(branch => branch.startsWith('* '));
    return current ? current.slice(2) : branches.shift();
  } catch (err) {
    console.error(`Failed to fetch branch for ${point}: `, err);
    throw err;
  }
};

const getAllBranches = async () => {
  try {
    const branches = await runGit(['branch', '--remote']);
    return branches.split('\n')
      .map(branch => branch.trim())
      .filter(branch => !!branch)
      .filter(branch => branch !== 'origin/HEAD -> origin/main')
      .sort();
  } catch (err) {
    console.error('Failed to fetch all branches');
    throw err;
  }
};

const getStabilizationBranches = async () => {
  return (await getAllBranches()).filter(branch => /^origin\/\d+-x-y$/.test(branch));
};

const getPreviousStabilizationBranch = async (current: string) => {
  const stabilizationBranches = (await getStabilizationBranches())
    .filter(branch => branch !== current && branch !== `origin/${current}`);

  if (!valid(current)) {
    // since we don't seem to be on a stabilization branch right now,
    // pick a placeholder name that will yield the newest branch
    // as a comparison point.
    current = 'v999.999.999';
  }

  let newestMatch = null;
  for (const branch of stabilizationBranches) {
    if (gte(semverify(branch), semverify(current))) {
      continue;
    }
    if (newestMatch && lte(semverify(branch), semverify(newestMatch))) {
      continue;
    }
    newestMatch = branch;
  }
  return newestMatch!;
};

const getPreviousPoint = async (point: string) => {
  const currentBranch = await getBranchOf(point);
  const currentTag = (await getTagsOf(point)).filter(tag => tagIsSupported(tag)).pop()!;
  const currentIsStable = tagIsStable(currentTag);

  try {
    // First see if there's an earlier tag on the same branch
    // that can serve as a reference point.
    let tags = (await getTagsOnBranch(`${point}^`)).filter(tag => tagIsSupported(tag));
    if (currentIsStable) {
      tags = tags.filter(tag => tagIsStable(tag));
    }
    if (tags.length) {
      return tags.pop();
    }
  } catch (error) {
    console.log('error', error);
  }

  // Otherwise, use the newest stable release that precedes this branch.
  // To reach that you may have to walk past >1 branch, e.g. to get past
  // 2-1-x which never had a stable release.
  let branch = currentBranch;
  while (branch) {
    const prevBranch = await getPreviousStabilizationBranch(branch);
    const tags = (await getTagsOnBranch(prevBranch)).filter(tag => tagIsStable(tag));
    if (tags.length) {
      return tags.pop();
    }
    branch = prevBranch;
  }
};

async function getReleaseNotes (range: string, newVersion?: string, unique?: boolean) {
  const rangeList = range.split('..') || ['HEAD'];
  const to = rangeList.pop()!;
  const from = rangeList.pop() || (await getPreviousPoint(to))!;

  if (!newVersion) {
    newVersion = to;
  }

  const notes = await get(from, to, newVersion);
  const ret: { text: string; warning?: string; } = {
    text: render(notes, unique)
  };

  if (notes.unknown.length) {
    ret.warning = `You have ${notes.unknown.length} unknown release notes. Please fix them before releasing.`;
  }

  return ret;
}

async function main () {
  const { values: { help, unique, version }, positionals } = parseArgs({
    options: {
      help: {
        type: 'boolean'
      },
      unique: {
        type: 'boolean'
      },
      version: {
        type: 'string'
      }
    },
    allowPositionals: true
  });

  const range = positionals.shift();
  if (help || !range) {
    const name = basename(process.argv[1]);
    console.log(`
easy usage: ${name} version

full usage: ${name} [begin..]end [--version version] [--unique]

 * 'begin' and 'end' are two git references -- tags, branches, etc --
   from which the release notes are generated.
 * if omitted, 'begin' defaults to the previous tag in end's branch.
 * if omitted, 'version' defaults to 'end'. Specifying a version is
   useful if you're making notes on a new version that isn't tagged yet.
 * '--unique' omits changes that also landed in other branches.

For example, these invocations are equivalent:
  ${process.argv[1]} v4.0.1
  ${process.argv[1]} v4.0.0..v4.0.1 --version v4.0.1
`);
    return 0;
  }

  const notes = await getReleaseNotes(range, version, unique);
  console.log(notes.text);
  if (notes.warning) {
    throw new Error(notes.warning);
  }
}

if (require.main === module) {
  main().catch((err) => {
    console.error('Error Occurred:', err);
    process.exit(1);
  });
}

export default getReleaseNotes;
