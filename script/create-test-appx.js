/**
 * @file Creates an appx using the built Electron
 *
 * ## Usage
 *
 * Take ./out/Debug and turn it into an appx
 * > node ./script/create-test-appx.js
 *
 * Take ./out/Release and turn it into an appx
 * > node ./script/create-test-appx.js ./out/Release
 *
 * ## Certificate
 *
 * This script will automatically detect whether or not the development
 * certificate is installed. If it isn't, it will attempt to install it.
 * If installation of the certificate fails, you likely don't have enough
 * rights and need to restart the script "as administrator".
 */

const fs = require('fs-extra')
const path = require('path')
const childProcess = require('child_process')
const electronWindowsStore = require('electron-windows-store')

// Parse arguments
const args = process.argv.slice(2)

// Constants
const BUILD_PATH = args[0]
  ? path.join(process.cwd(), args[0])
  : path.join(__dirname, `../../out/Debug`)
const STAGING_PATH = path.join(__dirname, '../../out/AppContainerStaging')
const OUT_PATH = path.join(__dirname, '../../out/AppContainer')
const CERT_PATH = path.join(__dirname, 'appcontainer/electrontest.pfx')
const ASSETS_PATH = path.join(__dirname, 'appcontainer/Assets')
const MANIFEST_PATH = path.join(__dirname, 'appcontainer/appxmanifest.xml')
const SHIM_PATH = path.join(__dirname, 'appcontainer/shim')
const PREAPPX_PATH = path.join(OUT_PATH, 'pre-appx')
const WINDOWS_KIT_PATH = `C:\\Program Files (x86)\\Windows Kits\\10\\bin\\10.0.17134.0\\x64`

console.log(`Packaging ${BUILD_PATH} as appx`)

/**
 * Do we need to copy this file over?
 *
 * @param {String} name
 * @returns {boolean}
 */
function isFileNeeded (name) {
  return !/.*\.(?:(pdb|ilk|exp|lib))$/ig.test(name)
}

/**
 * Do we need to copy this directory over?
 *
 * @param {String} name
 * @returns {boolean}
 */
function isDirNeeded (name) {
  return name === 'resources' || name === 'locales'
}

/**
 * Starts a child process using the provided executable
 *
 * @param fileName      - Path to the executable to start
 * @param args          - Arguments for spawn
 * @param options       - Options passed to spawn
 * @returns {Promise}   - A promise that resolves when the
 *                      process exits
 */
function executeChildProcess (fileName, args, options) {
  return new Promise((resolve, reject) => {
    const child = childProcess.spawn(fileName, args, options)
    const data = []
    const onData = (line) => {
      data.push(line)
      console.log(line.toString())
    }

    child.stdout.on('data', onData)
    child.stderr.on('data', onData)

    child.on('exit', (code) => {
      if (code !== 0) {
        return reject(new Error(fileName + ' exited with code: ' + code))
      }
      return resolve(data.join('\n'))
    })

    child.stdin.end()
  })
}

/**
 * Installs the test certificate to the local machine.
 * Needs admin rights.
 */
async function installPfx () {
  const installPfxArgs = [
    'Import-PfxCertificate',
    '-FilePath',
    CERT_PATH,
    '-CertStoreLocation',
    '"Cert:\\LocalMachine\\TrustedPeople"'
  ]

  await executeChildProcess('powershell.exe', installPfxArgs)
}

/**
 * Is the PFX certificate installed on the current machine?
 *
 * @returns {boolean}
 */
async function isPfxInstalled () {
  const checkPfxArgs = [
    '[bool](dir cert:\\LocalMachine\\TrustedPeople\\ | ? { $_.subject -like "cn=electrontest" })'
  ]

  const result = await executeChildProcess('powershell.exe', checkPfxArgs)

  return result.toLowerCase().trim() === 'true'
}

/**
 * Copy in our known shim files
 *
 * @returns {Promise<void>}
 */
async function copyInShim () {
  const contents = await fs.readdir(SHIM_PATH)

  for (const file of contents) {
    const source = path.join(SHIM_PATH, file)
    const desination = path.join(PREAPPX_PATH, file)

    await fs.copy(source, desination)
  }
}

/**
 * Take the output directory and turn it into an appx
 *
 * @returns {Promise<void>}
 */
async function convert () {
  fs.emptyDirSync(OUT_PATH)

  // Install the test certificate
  if (!(await isPfxInstalled())) {
    const warning = `The test certificate is not installed!\n` +
      `We will now try to install it. If this is not an administrative shell,\n` +
      `this operation will fail. If that's the case, please run this script\n` +
      `again as administrator.`

    console.warning(warning)

    await installPfx()
  }

  await electronWindowsStore({
    inputDirectory: STAGING_PATH,
    outputDirectory: OUT_PATH,
    packageVersion: '1.0.0.0',
    packageName: 'electron',
    manifest: MANIFEST_PATH,
    assets: ASSETS_PATH,
    devCert: CERT_PATH,
    createPriParams: ['/IndexName', '99999D7F.Electron'],
    makePri: true,
    windowsKit: WINDOWS_KIT_PATH,
    finalSay: copyInShim
  })
}

/**
 * Remove staging directories
 *
 * @returns {Promise<void>}
 */
async function cleanup () {
  await fs.remove(STAGING_PATH)
}

/**
 * Main script method: Create an appx
 *
 * @returns {Promise<void>}
 */
async function main () {
  if (!fs.existsSync(BUILD_PATH)) {
    console.log(`Cannot find ${BUILD_PATH}`)
    return
  }

  fs.emptyDirSync(STAGING_PATH)

  const dirContents = fs.readdirSync(BUILD_PATH)

  // Copy things over
  for (const item of dirContents) {
    const source = path.join(BUILD_PATH, item)
    const destination = path.join(STAGING_PATH, item)
    const itemStat = fs.statSync(source)
    const isNeededFile = itemStat.isFile() && isFileNeeded(item)
    const isNeededDir = itemStat.isDirectory() && isDirNeeded(item)

    if (isNeededFile || isNeededDir) {
      console.log(`Copying ${item}`)

      await fs.copy(source, destination)
    }
  }

  console.log(`\nCopying done, now converting\n`)

  await convert()

  console.log(`\nConversion done, now cleaning up\n`)

  await cleanup()

  console.log(`\nAll done\n`)
}

main()
