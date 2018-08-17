const { GitProcess } = require('dugite')
const path = require('path')
const semver = require('semver')
const gitDir = path.resolve(__dirname, '..')

async function determineNextMajorForMaster () {
  let branchNames
  let result = await GitProcess.exec(['branch', '-a', '--remote', '--list', 'origin/[0-9]-[0-9]-x'], gitDir)
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
  const next = converted.reduce((v1, v2) => {
    return semver.gt(v1, v2) ? v1 : v2
  })
  return parseInt(next.split('.')[0], 10)
}

determineNextMajorForMaster().then(console.info).catch((err) => {
  console.error(err)
  process.exit(1)
})
