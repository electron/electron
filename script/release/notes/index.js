#!/usr/bin/env node

const { GitProcess } = require('dugite');
const minimist = require('minimist');
const path = require('path');
const semver = require('semver');

const { ELECTRON_DIR } = require('../../lib/utils');
const notesGenerator = require('./notes.js');

const semverify = version => version.replace(/^origin\//, '').replace(/[xy]/g, '0').replace(/-/g, '.');

const runGit = async (args) => {
  const response = await GitProcess.exec(args, ELECTRON_DIR);
  if (response.exitCode !== 0) {
    throw new Error(response.stderr.trim());
  }
  return response.stdout.trim();
};

const tagIsSupported = tag => tag && !tag.includes('nightly') && !tag.includes('unsupported');
const tagIsBeta = tag => tag.includes('beta');
const tagIsStable = tag => tagIsSupported(tag) && !tagIsBeta(tag);

const getTagsOf = async (point) => {
  return (await runGit(['tag', '--merged', point]))
    .split('\n')
    .map(tag => tag.trim())
    .filter(tag => semver.valid(tag))
    .sort(semver.compare);
};

const getTagsOnBranch = async (point) => {
  const masterTags = await getTagsOf('master');
  if (point === 'master') {
    return masterTags;
  }

  const masterTagsSet = new Set(masterTags);
  return (await getTagsOf(point)).filter(tag => !masterTagsSet.has(tag));
};

const getBranchOf = async (point) => {
  const branches = (await runGit(['branch', '-a', '--contains', point]))
    .split('\n')
    .map(branch => branch.trim())
    .filter(branch => !!branch);
  const current = branches.find(branch => branch.startsWith('* '));
  return current ? current.slice(2) : branches.shift();
};

const getAllBranches = async () => {
  return (await runGit(['branch', '--remote']))
    .split('\n')
    .map(branch => branch.trim())
    .filter(branch => !!branch)
    .filter(branch => branch !== 'origin/HEAD -> origin/master')
    .sort();
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

async function getReleaseNotes (range, newVersion, explicitLinks) {
  const rangeList = range.split('..') || ['HEAD'];
  const to = rangeList.pop();
  const from = rangeList.pop() || (await getPreviousPoint(to));

  if (!newVersion) {
    newVersion = to;
  }

  console.log(`Generating release notes between ${from} and ${to} for version ${newVersion}`);
  const notes = await notesGenerator.get(from, to, newVersion);
  const ret = {
    text: notesGenerator.render(notes, explicitLinks)
  };

  if (notes.unknown.length) {
    ret.warning = `You have ${notes.unknown.length} unknown release notes. Please fix them before releasing.`;
  }

  return ret;
}

async function main () {
  const opts = minimist(process.argv.slice(2), {
    boolean: ['explicit-links', 'help'],
    string: ['version']
  });
  opts.range = opts._.shift();
  if (opts.help || !opts.range) {
    const name = path.basename(process.argv[1]);
    console.log(`
easy usage: ${name} version

full usage: ${name} [begin..]end [--version version] [--explicit-links]

 * 'begin' and 'end' are two git references -- tags, branches, etc --
   from which the release notes are generated.
 * if omitted, 'begin' defaults to the previous tag in end's branch.
 * if omitted, 'version' defaults to 'end'. Specifying a version is
   useful if you're making notes on a new version that isn't tagged yet.
 * 'explicit-links' makes every note's issue, commit, or pull an MD link

For example, these invocations are equivalent:
  ${process.argv[1]} v4.0.1
  ${process.argv[1]} v4.0.0..v4.0.1 --version v4.0.1
`);
    return 0;
  }

  const notes = await getReleaseNotes(opts.range, opts.version, opts['explicit-links']);
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
