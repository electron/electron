const OUT_DIR = process.env.ELECTRON_OUT_DIR || 'Debug'

const { GitProcess } = require('dugite')
const path = require('path')

require('colors')
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

async function handleGitCall (args, gitDir) {
  const details = await GitProcess.exec(args, gitDir)
  if (details.exitCode === 0) {
    return details.stdout.replace(/^\*|\s+|\s+$/, '')
  } else {
    const error = GitProcess.parseError(details.stderr)
    console.log(`${fail} couldn't parse git process call: `, error)
    process.exit(1)
  }
}

async function getCurrentBranch (gitDir) {
  let branch = await handleGitCall(['rev-parse', '--abbrev-ref', 'HEAD'], gitDir)
  if (branch !== 'master' && !branch.match(/[0-9]+-[0-9]+-x/)) {
    const lastCommit = await handleGitCall(['rev-parse', 'HEAD'], gitDir)
    const branches = (await handleGitCall([
      'branch',
      '--contains',
      lastCommit,
      '--remote'
    ], gitDir)).split('\n')

    branch = branches.filter(b => b.trim() === 'master' || b.match(/[0-9]+-[0-9]+-x/))[0]
    if (!branch) {
      console.log(`${fail} no release branch exists for this ref`)
      process.exit(1)
    }
    if (branch.startsWith('origin/')) {
      branch = branch.substr('origin/'.length)
    }
  }
  return branch.trim()
}

module.exports = {
  getCurrentBranch,
  getElectronExec,
  getAbsoluteElectronExec,
  OUT_DIR
}
