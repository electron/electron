#!/usr/bin/env node

const { GitProcess } = require('dugite');
const minimist = require('minimist');
const path = require('path');
const semver = require('semver');

const { ELECTRON_DIR } = require('../../lib/utils');
const notesGenerator = require('./notes.js');

const { Octokit } = require('@octokit/rest');
const octokit = new Octokit({
  auth: process.env.ELECTRON_GITHUB_TOKEN
});

const semverify = version => version.replace(/^origin\//, '').replace(/[xy]/g, '0').replace(/-/g, '.');

const runGit = async (args) => {
  console.info(`Running: git ${args.join(' ')}`);
  const response = await GitProcess.exec(args, ELECTRON_DIR);
  if (response.exitCode !== 0) {
    throw new Error(response.stderr.trim());
  }
  return response.stdout.trim();
};

const tagIsSupported = tag => tag && !tag.includes('nightly') && !tag.includes('unsupported');
const tagIsAlpha = tag => tag && tag.includes('alpha');
const tagIsBeta = tag => tag && tag.includes('beta');
const tagIsStable = tag => tagIsSupported(tag) && !tagIsBeta(tag) && !tagIsAlpha(tag);

const getTagsOf = async (point) => {
  try {
    const tags = await runGit(['tag', '--merged', point]);
    return tags.split('\n')
      .map(tag => tag.trim())
      .filter(tag => semver.valid(tag))
      .sort(semver.compare);
  } catch (err) {
    console.error(`Failed to fetch tags for point ${point}`);
    throw err;
  }
};

const getTagsOnBranch = async (point) => {
  const { data: { default_branch: defaultBranch } } = await octokit.repos.get({
    owner: 'electron',
    repo: 'electron'
  });
  const mainTags = await getTagsOf(defaultBranch);
  if (point === defaultBranch) {
    return mainTags;
  }

  const mainTagsSet = new Set(mainTags);
  return (await getTagsOf(point)).filter(tag => !mainTagsSet.has(tag));
};

const getBranchOf = async (point) => {
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
      // TODO(main-migration): Simplify once branch rename is complete.
      .filter(branch => branch !== 'origin/HEAD -> origin/master' && branch !== 'origin/HEAD -> origin/main')
      .sort();
  } catch (err) {
    console.error('Failed to fetch all branches');
    throw err;
  }
};

const getStabilizationBranches = async () => {
  return (await getAllBranches())
    .filter(branch => /^origin\/\d+-\d+-x$/.test(branch) || /^origin\/\d+-x-y$/.test(branch));
};

const getPreviousStabilizationBranch = async (current) => {
  const stabilizationBranches = (await getStabilizationBranches())
    .filter(branch => branch !== current && branch !== `origin/${current}`);

  if (!semver.valid(current)) {
    // since we don't seem to be on a stabilization branch right now,
    // pick a placeholder name that will yield the newest branch
    // as a comparison point.
    current = 'v999.999.999';
  }

  let newestMatch = null;
  for (const branch of stabilizationBranches) {
    if (semver.gte(semverify(branch), semverify(current))) {
      continue;
    }
    if (newestMatch && semver.lte(semverify(branch), semverify(newestMatch))) {
      continue;
    }
    newestMatch = branch;
  }
  return newestMatch;
};

const getPreviousPoint = async (point) => {
  const currentBranch = await getBranchOf(point);
  const currentTag = (await getTagsOf(point)).filter(tag => tagIsSupported(tag)).pop();
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

  // Otherwise, use the newest stable release that preceeds this branch.
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

async function getReleaseNotes (range, newVersion) {
  const rangeList = range.split('..') || ['HEAD'];
  const to = rangeList.pop();
  const from = rangeList.pop() || (await getPreviousPoint(to));

  if (!newVersion) {
    newVersion = to;
  }

  const notes = await notesGenerator.get(from, to, newVersion);
  const ret = {
    text: notesGenerator.render(notes)
  };

  if (notes.unknown.length) {
    ret.warning = `You have ${notes.unknown.length} unknown release notes. Please fix them before releasing.`;
  }

  return ret;
}

async function main () {
  const opts = minimist(process.argv.slice(2), {
    boolean: ['help'],
    string: ['version']
  });
  opts.range = opts._.shift();
  if (opts.help || !opts.range) {
    const name = path.basename(process.argv[1]);
    console.log(`
easy usage: ${name} version

full usage: ${name} [begin..]end [--version version]

 * 'begin' and 'end' are two git references -- tags, branches, etc --
   from which the release notes are generated.
 * if omitted, 'begin' defaults to the previous tag in end's branch.
 * if omitted, 'version' defaults to 'end'. Specifying a version is
   useful if you're making notes on a new version that isn't tagged yet.

For example, these invocations are equivalent:
  ${process.argv[1]} v4.0.1
  ${process.argv[1]} v4.0.0..v4.0.1 --version v4.0.1
`);
    return 0;
  }

  const notes = await getReleaseNotes(opts.range, opts.version);
  console.log(notes.text);
  if (notes.warning) {
    throw new Error(notes.warning);
  }
}

if (process.mainModule === module) {
  main().catch((err) => {
    console.error('Error Occurred:', err);
    process.exit(1);
  });
}

module.exports = getReleaseNotes;
