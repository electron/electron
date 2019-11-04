#!/usr/bin/env node

const { GitProcess } = require('dugite')
const fs = require('fs')
const semver = require('semver')
const path = require('path')
const { promisify } = require('util')
const minimist = require('minimist')

const { ELECTRON_DIR } = require('../lib/utils')
const versionUtils = require('./version-utils')

const writeFile = promisify(fs.writeFile)
const readFile = promisify(fs.readFile)

function parseCommandLine () {
  let help
  const opts = minimist(process.argv.slice(2), {
    string: [ 'bump', 'version' ],
    boolean: [ 'dryRun', 'help' ],
    alias: { 'version': ['v'] },
    unknown: arg => { help = true }
  })
  if (help || opts.help || !opts.bump) {
    console.log(`
      Bump release version number. Possible arguments:\n
        --bump=patch to increment patch version\n
        --version={version} to set version number directly\n
        --dryRun to print the next version without updating files
      Note that you can use both --bump and --stable  simultaneously. 
    `)
    process.exit(0)
  }
  return opts
}

// run the script
async function main () {
  const opts = parseCommandLine()
  const currentVersion = await versionUtils.getElectronVersion()
  const version = await nextVersion(opts.bump, currentVersion)

  const parsed = semver.parse(version)
  const components = {
    major: parsed.major,
    minor: parsed.minor,
    patch: parsed.patch,
    pre: parsed.prerelease
  }

  // print would-be new version and exit early
  if (opts.dryRun) {
    console.log(`new version number would be: ${version}\n`)
    return 0
  }

  // update all version-related files
  await Promise.all([
    updateVersion(version),
    updatePackageJSON(version),
    updateWinRC(components)
  ])

  // commit all updated version-related files
  await commitVersionBump(version)

  console.log(`Bumped to version: ${version}`)
}

// get next version for release based on [nightly, beta, stable]
async function nextVersion (bumpType, version) {
  if (versionUtils.isNightly(version) || versionUtils.isBeta(version)) {
    switch (bumpType) {
      case 'nightly':
        version = await versionUtils.nextNightly(version)
        break
      case 'beta':
        version = await versionUtils.nextBeta(version)
        break
      case 'stable':
        version = semver.valid(semver.coerce(version))
        break
      default:
        throw new Error('Invalid bump type.')
    }
  } else if (versionUtils.isStable(version)) {
    switch (bumpType) {
      case 'nightly':
        version = versionUtils.nextNightly(version)
        break
      case 'beta':
        throw new Error('Cannot bump to beta from stable.')
      case 'minor':
        version = semver.inc(version, 'minor')
        break
      case 'stable':
        version = semver.inc(version, 'patch')
        break
      default:
        throw new Error('Invalid bump type.')
    }
  } else {
    throw new Error(`Invalid current version: ${version}`)
  }
  return version
}

// update VERSION file with latest release info
async function updateVersion (version) {
  const versionPath = path.resolve(ELECTRON_DIR, 'ELECTRON_VERSION')
  await writeFile(versionPath, version, 'utf8')
}

// update package metadata files with new version
async function updatePackageJSON (version) {
  const filePath = path.resolve(ELECTRON_DIR, 'package.json')
  const file = require(filePath)
  file.version = version
  await writeFile(filePath, JSON.stringify(file, null, 2))
}

// push bump commit to release branch
async function commitVersionBump (version) {
  const gitArgs = ['commit', '-a', '-m', `Bump v${version}`, '-n']
  await GitProcess.exec(gitArgs, ELECTRON_DIR)
}

// updates atom.rc file with new semver values
async function updateWinRC (components) {
  const filePath = path.resolve(ELECTRON_DIR, 'shell', 'browser', 'resources', 'win', 'atom.rc')
  const data = await readFile(filePath, 'utf8')
  const arr = data.split('\n')
  arr.forEach((line, idx) => {
    if (line.includes('FILEVERSION')) {
      arr[idx] = ` FILEVERSION ${versionUtils.makeVersion(components, ',', versionUtils.preType.PARTIAL)}`
      arr[idx + 1] = ` PRODUCTVERSION ${versionUtils.makeVersion(components, ',', versionUtils.preType.PARTIAL)}`
    } else if (line.includes('FileVersion')) {
      arr[idx] = `            VALUE "FileVersion", "${versionUtils.makeVersion(components, '.')}"`
      arr[idx + 5] = `            VALUE "ProductVersion", "${versionUtils.makeVersion(components, '.')}"`
    }
  })
  await writeFile(filePath, arr.join('\n'))
}

if (process.mainModule === module) {
  main().catch((error) => {
    console.error(error)
    process.exit(1)
  })
}

module.exports = { nextVersion }
