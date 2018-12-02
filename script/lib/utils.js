const OUT_DIR = process.env.ELECTRON_OUT_DIR || 'Debug'

const { GitProcess } = require('dugite')
const path = require('path')

require('colors')
const pass = '\u2713'.green
const fail = '\u2717'.red

function getElectronExec () {
  switch (process.platform) {
    case 'darwin':
      return `out/${OUT_DIR}/Electron.app/Contents/MacOS/Electron`
    case 'win32':
      return `out/${OUT_DIR}/electron.exe`
    case 'linux':
      return `out/${OUT_DIR}/electron`
    default:
      throw new Error('Unknown platform')
  }
}

function getAbsoluteElectronExec () {
  return path.resolve(__dirname, '../../..', getElectronExec())
}

async function getCurrentBranch (gitDir) {
  const gitArgs = ['rev-parse', '--abbrev-ref', 'HEAD']
  const branchDetails = await GitProcess.exec(gitArgs, gitDir)
  if (branchDetails.exitCode === 0) {
    const currentBranch = branchDetails.stdout.trim()
    console.log(`${pass} current git branch is: ${currentBranch}`)
    return currentBranch
  } else {
    const error = GitProcess.parseError(branchDetails.stderr)
    console.log(`${fail} couldn't get details current branch: `, error)
    process.exit(1)
  }
}

module.exports = {
  getCurrentBranch,
  getElectronExec,
  getAbsoluteElectronExec,
  OUT_DIR
}
