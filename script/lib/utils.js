const { GitProcess } = require('dugite')
const fs = require('fs')
const path = require('path')

const ELECTRON_DIR = path.resolve(__dirname, '..', '..')
const SRC_DIR = path.resolve(ELECTRON_DIR, '..')

require('colors')
const fail = '\u2717'.red

// Returns the build path for a given build type (ex: '/path/to/electron-gn/src/out/Testing').
// If the ELECTRON_OUT_DIR env var is set (ex: 'Testing'), it's used as the build type.
// Otherwise, look in `out/` for well-known build types and return the first hit.
function getOutPath (shouldLog = false) {
  const buildOutPath = buildType => path.join(SRC_DIR, 'out', buildType)

  if (process.env.ELECTRON_OUT_DIR) {
    const outPath = buildOutPath(process.env.ELECTRON_OUT_DIR)
    if (shouldLog) console.log(`Using ${outPath}`)
    return outPath
  }

  const buildTypes = ['Debug', 'Testing', 'Release']
  const outPath = buildTypes.map(buildOutPath).find(fs.existsSync)
  if (!outPath) throw new Error(`No build directory found. (Is ELECTRON_OUT_DIR set?)`)
  if (shouldLog) console.log(`Using ${outPath}`)
  return outPath
}

// Returns the Electron executable's path (ex: '/path/to/electron-gn/src/out/Testing/electron.exe').
// The build type (ex: 'Testing') is determined using the same process as getOutPath().
function getElectronExecPath (shouldLog = false) {
  const execName = (() => {
    switch (process.platform) {
      case 'linux': return 'electron'
      case 'win32': return 'electron.exe'
      case 'darwin': return path.join('Electron.app', 'Contents', 'MacOS', 'Electron')
      default: throw new Error(`Unrecognized platform '${process.platform}'`)
    }
  })()
  const execPath = path.join(getOutPath(), execName)
  if (shouldLog) console.log(`Using ${execPath}`)
  return execPath
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
    if (branch.startsWith('origin/')) branch = branch.substr('origin/'.length)
  }
  return branch.trim()
}

module.exports = {
  getCurrentBranch,
  getOutPath,
  getElectronExecPath,
  ELECTRON_DIR,
  SRC_DIR
}
