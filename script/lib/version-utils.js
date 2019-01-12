const path = require('path')
const fs = require('fs')
const semver = require('semver')
const { getLastMajorForMaster } = require('../get-last-major-for-master')
const { GitProcess } = require('dugite')
const { promisify } = require('util')

const readFile = promisify(fs.readFile)
const gitDir = path.resolve(__dirname, '..', '..')

const preType = {
  NONE: 'none',
  PARTIAL: 'partial',
  FULL: 'full'
}

const getCurrentDate = () => {
  const d = new Date()
  const dd = `${d.getDate()}`.padStart(2, '0')
  const mm = `${d.getMonth() + 1}`.padStart(2, '0')
  const yyyy = d.getFullYear()
  return `${yyyy}${mm}${dd}`
}

const isNightly = v => v.includes('nightly')
const isBeta = v => v.includes('beta')
const isStable = v => {
  const parsed = semver.parse(v)
  return !!(parsed && parsed.prerelease.length === 0)
}

const makeVersion = (components, delim, pre = preType.NONE) => {
  let version = [components.major, components.minor, components.patch].join(delim)
  if (pre === preType.PARTIAL) {
    version += `${delim}${components.pre[1]}`
  } else if (pre === preType.FULL) {
    version += `-${components.pre[0]}${delim}${components.pre[1]}`
  }
  return version
}

async function nextBeta (v) {
  const next = semver.coerce(semver.clean(v))

  const tagBlob = await GitProcess.exec(['tag', '--list', '-l', `v${next}-beta.*`], gitDir)
  const tags = tagBlob.stdout.split('\n').filter(e => e !== '')
  tags.sort((t1, t2) => semver.gt(t1, t2))

  // increment the latest existing beta tag or start at beta.1 if it's a new beta line
  return tags.length === 0 ? semver.inc(next, 'beta', 'prerelease') : semver.inc(tags.pop(), 'prerelease')
}

async function getElectronVersion () {
  const versionPath = path.join(__dirname, '..', '..', 'ELECTRON_VERSION')
  const version = await readFile(versionPath, 'utf8')
  return version.trim()
}

async function nextNightly (v) {
  let next = semver.valid(semver.coerce(v))
  const pre = `nightly.${getCurrentDate()}`

  const branch = (await GitProcess.exec(['rev-parse', '--abbrev-ref', 'HEAD'], gitDir)).stdout.trim()
  if (branch === 'master') {
    next = semver.inc(await getLastMajorForMaster(), 'major')
  } else if (isStable(v)) {
    next = semver.inc(next, 'patch')
  }

  return `${next}-${pre}`
}

module.exports = {
  isStable,
  isBeta,
  isNightly,
  nextBeta,
  makeVersion,
  getElectronVersion,
  nextNightly,
  preType
}
