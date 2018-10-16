#!/usr/bin/env node

const { GitProcess } = require('dugite')
const path = require('path')
const semver = require('semver')

const notes = require(path.resolve(__dirname, 'notes.js'))

const gitDir = path.resolve(__dirname, '..', '..')

const semverify = v => v.replace(/^origin\//, '').replace('x', '0').replace(/-/g, '.')

const runGit = async (args) => {
  const response = await GitProcess.exec(args, gitDir)
  if (response.exitCode !== 0) throw new Error(response.stderr.trim())
  return response.stdout.trim()
}

const tagIsSupported = t => t && !t.includes('nightly') && !t.includes('unsupported')
const tagIsBeta = t => t.includes('beta')
const tagIsStable = t => tagIsSupported(t) && !tagIsBeta(t)

const getTagsOf = async (point) => {
  return (await runGit(['tag', '--merged', point]))
    .split('\n')
    .map(t => t.trim())
    .filter(t => !!t)
    .sort(semver.compare)
}

const getTagsOnBranch = async (point) => {
  const m = await getTagsOf('master')
  if (point === 'master') return m
  const masterTags = new Set(m)
  return (await getTagsOf(point)).filter(t => !masterTags.has(t))
}

const getBranchOf = async (point) => {
  return (await runGit(['branch', '--remote', '--contains', point]))
    .split('\n')
    .map(b => b.trim())
    .filter(b => !!b)
    .shift()
}

const getAllBranches = async () => {
  return (await runGit(['branch', '--remote']))
    .split('\n')
    .map(b => b.trim())
    .filter(b => !!b)
    .filter(branch => branch !== 'origin/HEAD -> origin/master')
    .sort()
}

const getStabilizationBranches = async () => {
  return (await getAllBranches())
    .filter(b => /^origin\/\d+-\d+-x$/.test(b))
}

const getPreviousStabilizationBranch = async (current) => {
  const stabilizationBranches = (await getStabilizationBranches())
    .filter(b => b !== current && b !== `origin/${current}`)

  let previous = null
  for (const b of stabilizationBranches) {
    if (semver.gte(semverify(b), semverify(current))) continue
    if (previous && semver.lte(semverify(b), semverify(previous))) continue
    previous = b
  }
  return previous
}

const getPreviousPoint = async (point) => {
  const currentBranch = await getBranchOf(point)
  const currentTag = (await getTagsOf(point)).filter(t => tagIsSupported(t)).pop()
  const currentIsStable = tagIsStable(currentTag)
  // console.log({currentBranch, currentTag, currentIsStable})
  try {
    // First see if there's an earlier tag on the same branch
    // that can serve as a reference point.
    let tags = (await getTagsOnBranch(`${point}^`)).filter(t => tagIsSupported(t))
    if (currentIsStable) tags = tags.filter(t => tagIsStable(t))
    if (tags.length) return tags.pop()
  } catch (e) {
    console.log('error', e)
  }

  // Otherwise, use the newest stable release that preceeds this branch.
  // You may have to walk >1 branch, e.g. to get past 2-1-x which never had
  // a stable release.
  let branch = currentBranch
  while (branch) {
    const prevBranch = await getPreviousStabilizationBranch(branch)
    const tags = (await getTagsOnBranch(prevBranch)).filter(t => tagIsStable(t))
    // console.log({prevBranch, tags})
    if (tags.length) return tags.pop()
    branch = prevBranch
  }
}

async function main () {
  if (process.argv.length !== 3) {
    console.log('Use: script/release-notes/index.js {tag | tag1..tag2}')
    return 1
  }

  const opts = {}
  const range = process.argv.length === 3 ? process.argv[2].split('..') : ['HEAD']
  opts.to = range.pop()
  opts.from = range.pop() || (await getPreviousPoint(opts.to))

  const n = await notes.get(opts.from, opts.to)
  console.log(notes.render(n))
  if (n.unknown.length) {
    throw new Error(`You have ${n.unknown.length} unknown release notes. Please fix them before releasing.`)
  }
}

if (process.mainModule === module) {
  main().catch((err) => {
    console.error('Error Occurred:', err)
    process.exit(1)
  })
}
