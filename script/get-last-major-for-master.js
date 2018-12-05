const { GitProcess } = require('dugite')
const path = require('path')
const semver = require('semver')
const gitDir = path.resolve(__dirname, '..')

async function getLastMajorForMaster () {
  let branchNames
  const result = await GitProcess.exec(['branch', '-a', '--remote', '--list', 'origin/[0-9]-[0-9]-x'], gitDir)
  if (result.exitCode === 0) {
    branchNames = result.stdout.trim().split('\n')
    const filtered = branchNames.map(b => b.replace('origin/', ''))
    return getNextReleaseBranch(filtered)
  } else {
    throw new Error('Release branches could not be fetched.')
  }
}

function getNextReleaseBranch (branches) {
  const converted = branches.map(b => b.replace(/-/g, '.').replace('x', '0'))
  return converted.reduce((v1, v2) => semver.gt(v1, v2) ? v1 : v2)
}

module.exports = { getLastMajorForMaster }
