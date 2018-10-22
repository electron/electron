const fs = require('fs-extra')
const path = require('path')
const childProcess = require('child_process')
const electronWindowsStore = require('electron-windows-store')

const DEBUG_PATH = path.join(__dirname, '../out/D')
const STAGING_PATH = path.join(__dirname, '../out/AppContainerStaging')
const OUT_PATH = path.join(__dirname, '../out/AppContainer')
const CERT_PATH = path.join(__dirname, 'appcontainer/electrontest.pfx')
const ASSETS_PATH = path.join(__dirname, 'appcontainer/Assets')
const MANIFEST_PATH = path.join(__dirname, 'appcontainer/appxmanifest.xml')
const SHIM_PATH = path.join(__dirname, 'appcontainer/shim')
const PREAPPX_PATH = path.join(OUT_PATH, 'pre-appx')

/**
 * Do we need to copy this file over?
 *
 * @param {String} name
 * @returns {boolean}
 */
function isFileNeeded (name) {
  return !/.*\.(?:(pdb)|(ilk)|(exp)|(lib))$/ig.test(name)
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

    child.stdout.on('data', (line) => {
      data.push(line)
      console.log(line.toString())
    })
    child.stderr.on('data', (line) => {
      data.push(line)
      console.log(line.toString())
    })

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

async function isPfxInstalled () {
  const checkPfxArgs = [
    '[bool](dir cert:\\LocalMachine\\TrustedPeople\\ | ? { $_.subject -like "cn=electrontest" })'
  ]

  const result = await executeChildProcess('powershell.exe', checkPfxArgs)

  return result.toLowerCase().trim() === 'true'
}

async function copyInShim () {
  const contents = await fs.readdir(SHIM_PATH)

  for (const file of contents) {
    const source = path.join(SHIM_PATH, file)
    const desination = path.join(PREAPPX_PATH, file)

    await fs.copy(source, desination)
  }
}

async function main () {
  if (!fs.existsSync(DEBUG_PATH)) {
    console.log(`Cannot find ${DEBUG_PATH}`)
    return
  }

  fs.emptyDirSync(STAGING_PATH)

  const dirContents = fs.readdirSync(DEBUG_PATH)

  // Copy things over
  for (const item of dirContents) {
    const source = path.join(DEBUG_PATH, item)
    const destination = path.join(STAGING_PATH, item)
    const itemStat = fs.statSync(source)
    const isNeededFile = itemStat.isFile() && isFileNeeded(item)
    const isNeededDir = !isNeededFile && isDirNeeded(item)

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

async function convert () {
  fs.emptyDirSync(OUT_PATH)

  // Install the test certificate
  if (!(await isPfxInstalled())) {
    console.warn(`The test certificate is not installed!`)
    console.warn(`We will now try to install it. If this is not an administrative shell,`)
    console.warn(`this operation will fail. If that's the case, please run this script`)
    console.warn(`again as administrator`)

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
    finalSay: copyInShim
  })
}

async function cleanup () {
  await fs.remove(STAGING_PATH)
}

main()
