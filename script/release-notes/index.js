#!/usr/bin/env node

const { GitProcess } = require('dugite')
const path = require('path')
const semver = require('semver')

const notesGenerator = require('./notes.js')

const gitDir = path.resolve(__dirname, '..', '..')

const semverify = version => version.replace(/^origin\//, '').replace('x', '0').replace(/-/g, '.')

const runGit = async (args) => {
  const response = await GitProcess.exec(args, gitDir)
  if (response.exitCode !== 0) {
    throw new Error(response.stderr.trim())
  }
  return response.stdout.trim()
}

const tagIsSupported = tag => tag && !tag.includes('nightly') && !tag.includes('unsupported')
const tagIsBeta = tag => tag.includes('beta')
const tagIsStable = tag => tagIsSupported(tag) && !tagIsBeta(tag)

const getTagsOf = async (point) => {
  return (await runGit(['tag', '--merged', point]))
    .split('\n')
    .map(tag => tag.trim())
    .filter(tag => semver.valid(tag))
    .sort(semver.compare)
}

const getTagsOnBranch = async (point) => {
  const masterTags = await getTagsOf('master')
  if (point === 'master') {
    return masterTags
  }

  const masterTagsSet = new Set(masterTags)
  return (await getTagsOf(point)).filter(tag => !masterTagsSet.has(tag))
}

const getBranchOf = async (point) => {
  const branches = (await runGit(['branch', '-a', '--contains', point]))
    .split('\n')
    .map(branch => branch.trim())
    .filter(branch => !!branch)
  const current = branches.find(branch => branch.startsWith('* '))
  return current ? current.slice(2) : branches.shift()
}

const getAllBranches = async () => {
  return (await runGit(['branch', '--remote']))
    .split('\n')
    .map(branch => branch.trim())
    .filter(branch => !!branch)
    .filter(branch => branch !== 'origin/HEAD -> origin/master')
    .sort()
}

const getStabilizationBranches = async () => {
  return (await getAllBranches())
    .filter(branch => /^origin\/\d+-\d+-x$/.test(branch))
}

const getPreviousStabilizationBranch = async (current) => {
  const stabilizationBranches = (await getStabilizationBranches())
    .filter(branch => branch !== current && branch !== `origin/${current}`)

  if (!semver.valid(current)) {
    // since we don't seem to be on a stabilization branch right now,
    // pick a placeholder name that will yield the newest branch
    // as a comparison point.
    current = 'v999.999.999'
  }

  let newestMatch = null
  for (const branch of stabilizationBranches) {
    if (semver.gte(semverify(branch), semverify(current))) {
      continue
    }
    if (newestMatch && semver.lte(semverify(branch), semverify(newestMatch))) {
      continue
    }
    newestMatch = branch
  }
  return newestMatch
}

const getPreviousPoint = async (point) => {
  const currentBranch = await getBranchOf(point)
  const currentTag = (await getTagsOf(point)).filter(tag => tagIsSupported(tag)).pop()
  const currentIsStable = tagIsStable(currentTag)

  try {
    // First see if there's an earlier tag on the same branch
    // that can serve as a reference point.
    let tags = (await getTagsOnBranch(`${point}^`)).filter(tag => tagIsSupported(tag))
    if (currentIsStable) {
      tags = tags.filter(tag => tagIsStable(tag))
    }
    if (tags.length) {
      return tags.pop()
    }
  } catch (error) {
    console.log('error', error)
  }

  // Otherwise, use the newest stable release that preceeds this branch.
  // To reach that you may have to walk past >1 branch, e.g. to get past
  // 2-1-x which never had a stable release.
  let branch = currentBranch
  while (branch) {
    const prevBranch = await getPreviousStabilizationBranch(branch)
    const tags = (await getTagsOnBranch(prevBranch)).filter(tag => tagIsStable(tag))
    if (tags.length) {
      return tags.pop()
    }
    branch = prevBranch
  }
}

async function getReleaseNotes (range, explicitLinks) {
  const rangeList = range.split('..') || ['HEAD']
  const to = rangeList.pop()
  const from = rangeList.pop() || (await getPreviousPoint(to))
  console.log(`Generating release notes between ${from} and ${to}`)

  const notes = await notesGenerator.get(from, to)
  const ret = {
    text: notesGenerator.render(notes, explicitLinks)
  }

  if (notes.unknown.length) {
    ret.warning = `You have ${notes.unknown.length} unknown release notes. Please fix them before releasing.`
  }

  return ret
}

async function main () {
  // TODO: minimist/commander
  const explicitLinks = process.argv.slice(2).some(arg => arg === '--explicit-links')
  if (process.argv.length > 4) {
    console.log('Use: script/release-notes/index.js [--explicit-links] [tag | tag1..tag2]')
    return 1
  }

  const range = process.argv[2] || 'HEAD'
  const notes = await getReleaseNotes(range, explicitLinks)
  console.log(notes.text)
  if (notes.warning) {
    throw new Error(notes.warning)
  }
}

if (process.mainModule === module) {
  main().catch((err) => {
    console.error('Error Occurred:', err)
    process.exit(1)
  })
}

module.exports = getReleaseNotes
