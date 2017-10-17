#!/usr/bin/env node

require('colors')
const args = require('minimist')(process.argv.slice(2))
const assert = require('assert')
const fs = require('fs')
const { execSync } = require('child_process')
const GitHub = require('github')
const { GitProcess } = require('dugite')
const nugget = require('nugget')
const pkg = require('../package.json')
const pkgVersion = `v${pkg.version}`
const pass = '\u2713'.green
const path = require('path')
const fail = '\u2717'.red
const sumchecker = require('sumchecker')
const temp = require('temp').track()
const { URL } = require('url')
let failureCount = 0

assert(process.env.ELECTRON_GITHUB_TOKEN, 'ELECTRON_GITHUB_TOKEN not found in environment')

const github = new GitHub()
github.authenticate({type: 'token', token: process.env.ELECTRON_GITHUB_TOKEN})
const gitDir = path.resolve(__dirname, '..')

async function getDraftRelease (version) {
  let releaseInfo = await github.repos.getReleases({owner: 'electron', repo: 'electron'})
  let drafts
  if (version) {
    drafts = releaseInfo.data
      .filter(release => release.tag_name === version)
  } else {
    drafts = releaseInfo.data
      .filter(release => release.draft)
  }
  check(drafts.length === 1, 'one draft exists', true)
  const draft = drafts[0]
  check(draft.tag_name === `v${pkg.version}`, `draft release version matches local package.json (v${pkg.version})`)
  check(draft.prerelease, 'draft is a prerelease')
  check(draft.body.length > 50 && !draft.body.includes('(placeholder)'), 'draft has release notes')
  return draft
}

async function validateReleaseAssets (draft) {
  const requiredAssets = assetsForVersion(draft.tag_name).sort()
  const extantAssets = draft.assets.map(asset => asset.name).sort()
  const downloadUrls = draft.assets.map(asset => asset.browser_download_url).sort()

  requiredAssets.forEach(asset => {
    check(extantAssets.includes(asset), asset)
  })
  check((failureCount === 0), `All required GitHub assets exist for release`, true)

  await verifyShasums(downloadUrls)
    .catch(err => {
      console.log(`${fail} error verifyingShasums`, err)
    })
  console.log(`Should be done verifying GitHub shasums`)
  const s3Urls = s3UrlsForVersion(draft.tag_name)
  await verifyShasums(s3Urls, true)
}

function check (condition, statement, exitIfFail = false) {
  if (condition) {
    console.log(`${pass} ${statement}`)
  } else {
    failureCount++
    console.log(`${fail} ${statement}`)
    if (exitIfFail) process.exit(1)
  }
}

function assetsForVersion (version) {
  const patterns = [
    'electron-{{VERSION}}-darwin-x64-dsym.zip',
    'electron-{{VERSION}}-darwin-x64-symbols.zip',
    'electron-{{VERSION}}-darwin-x64.zip',
    'electron-{{VERSION}}-linux-arm-symbols.zip',
    'electron-{{VERSION}}-linux-arm.zip',
    'electron-{{VERSION}}-linux-arm64-symbols.zip',
    'electron-{{VERSION}}-linux-arm64.zip',
    'electron-{{VERSION}}-linux-armv7l-symbols.zip',
    'electron-{{VERSION}}-linux-armv7l.zip',
    'electron-{{VERSION}}-linux-ia32-symbols.zip',
    'electron-{{VERSION}}-linux-ia32.zip',
    'electron-{{VERSION}}-linux-x64-symbols.zip',
    'electron-{{VERSION}}-linux-x64.zip',
    'electron-{{VERSION}}-mas-x64-dsym.zip',
    'electron-{{VERSION}}-mas-x64-symbols.zip',
    'electron-{{VERSION}}-mas-x64.zip',
    'electron-{{VERSION}}-win32-ia32-pdb.zip',
    'electron-{{VERSION}}-win32-ia32-symbols.zip',
    'electron-{{VERSION}}-win32-ia32.zip',
    'electron-{{VERSION}}-win32-x64-pdb.zip',
    'electron-{{VERSION}}-win32-x64-symbols.zip',
    'electron-{{VERSION}}-win32-x64.zip',
    'electron-api.json',
    'electron.d.ts',
    'ffmpeg-{{VERSION}}-darwin-x64.zip',
    'ffmpeg-{{VERSION}}-linux-arm.zip',
    'ffmpeg-{{VERSION}}-linux-arm64.zip',
    'ffmpeg-{{VERSION}}-linux-armv7l.zip',
    'ffmpeg-{{VERSION}}-linux-ia32.zip',
    'ffmpeg-{{VERSION}}-linux-x64.zip',
    'ffmpeg-{{VERSION}}-mas-x64.zip',
    'ffmpeg-{{VERSION}}-win32-ia32.zip',
    'ffmpeg-{{VERSION}}-win32-x64.zip',
    'SHASUMS256.txt'
  ]
  return patterns.map(pattern => pattern.replace(/{{VERSION}}/g, version))
}

function s3UrlsForVersion (version) {
  const bucket = 'https://gh-contractor-zcbenz.s3.amazonaws.com/'
  const patterns = [
    'atom-shell/dist/{{VERSION}}/iojs-{{VERSION}}-headers.tar.gz',
    'atom-shell/dist/{{VERSION}}/iojs-{{VERSION}}.tar.gz',
    'atom-shell/dist/{{VERSION}}/node-{{VERSION}}.tar.gz',
    'atom-shell/dist/{{VERSION}}/node.lib',
    'atom-shell/dist/{{VERSION}}/win-x64/iojs.lib',
    'atom-shell/dist/{{VERSION}}/win-x86/iojs.lib',
    'atom-shell/dist/{{VERSION}}/x64/node.lib',
    'atom-shell/dist/{{VERSION}}/SHASUMS.txt',
    'atom-shell/dist/{{VERSION}}/SHASUMS256.txt',
    'atom-shell/dist/index.json'
  ]
  return patterns.map(pattern => bucket + pattern.replace(/{{VERSION}}/g, version))
}

function checkVersion () {
  console.log('Verifying that app version matches package version.')
  let startScript = path.join(__dirname, 'start.py')
  let appVersion = runScript(startScript, ['--version'])
  check((appVersion === pkgVersion), `App version ${appVersion} matches ` +
    `package version ${pkgVersion}.`, true)
}

function runScript (scriptName, scriptArgs, cwd) {
  let scriptCommand = `${scriptName} ${scriptArgs.join(' ')}`
  let scriptOptions = {
    encoding: 'UTF-8'
  }
  if (cwd) {
    scriptOptions.cwd = cwd
  }
  try {
    return execSync(scriptCommand, scriptOptions)
  } catch (err) {
    console.log(`${fail} Error running ${scriptName}`, err)
    process.exit(1)
  }
}

function uploadNodeShasums () {
  console.log('Uploading Node SHASUMS file to S3.')
  let scriptPath = path.join(__dirname, 'upload-node-checksums.py')
  runScript(scriptPath, ['-v', pkgVersion])
  console.log(`${pass} Done uploading Node SHASUMS file to S3.`)
}

function uploadIndexJson () {
  console.log('Uploading index.json to S3.')
  let scriptPath = path.join(__dirname, 'upload-index-json.py')
  runScript(scriptPath, [])
  console.log(`${pass} Done uploading index.json to S3.`)
}

async function createReleaseShasums (release) {
  console.log(`Creating uploading the release SHASUMS.`)
  let scriptPath = path.join(__dirname, 'merge-electron-checksums.py')
  let checksums = runScript(scriptPath, ['-v', pkgVersion])
  let filePath = await saveShaSumFile(checksums)
  await uploadShasumFile(filePath, release)
}

async function uploadShasumFile (filePath, release) {
  let githubOpts = {
    owner: 'electron',
    repo: 'electron',
    id: release.id,
    filePath
  }
  return await github.repos.uploadAsset(githubOpts)
    .catch(err => {
      console.log(`{$fail} Error uploading ${filePath} to GitHub:`, err)
      process.exit(1)
    })
}

function saveShaSumFile (checksums) {
  return new Promise((resolve, reject) => {
    temp.open('SHASUMS256.txt', (err, info) => {
      if (err) {
        console.log(`${fail} Could not create SHASUMS256.txt file`)
        process.exit(1)
      } else {
        fs.writeFileSync(info.fd, checksums)
        fs.close(info.fd, (err) => {
          if (err) {
            console.log(`${fail} Could close SHASUMS256.txt file`)
            process.exit(1)
          }
          resolve(info.path)
        })
      }
    })
  })
}

async function publishRelease (release) {
  let githubOpts = {
    owner: 'electron',
    repo: 'electron',
    id: release.id,
    draft: false
  }
  return await github.repos.editRelease(githubOpts)
    .catch(err => {
      console.log(`{$fail} Error publishing release:`, err)
      process.exit(1)
    })
}

async function makeRelease (releaseToValidate) {
  if (releaseToValidate) {
    console.log(`Validating release ${args.validateRelease}`)
    let release = await getDraftRelease(args.validateRelease)
    await validateReleaseAssets(release)
  } else {
    checkVersion()
    let draftRelease = await getDraftRelease()
    uploadNodeShasums()
    uploadIndexJson()
    await createReleaseShasums(draftRelease)
    await validateReleaseAssets(draftRelease)
    await publishRelease()
    await cleanupReleaseBranch()
    console.log(`${pass} SUCCESS!!! Release has been published. Please run ` +
      `"npm run publish-to-npm" to publish release to npm.`)
  }
}

async function makeTempDir () {
  return new Promise((resolve, reject) => {
    temp.mkdir('electron-publish', (err, dirPath) => {
      if (err) {
        reject(err)
      } else {
        resolve(dirPath)
      }
    })
  })
}

function downloadFiles (urls, directory, quiet) {
  return new Promise((resolve, reject) => {
    let nuggetOpts = {
      dir: directory
    }
    if (quiet) {
      nuggetOpts.quiet = quiet
    }
    nugget(urls, nuggetOpts, (err) => {
      if (err) {
        reject(err)
      } else {
        resolve()
      }
    })
  })
}

async function verifyShasums (urls, isS3) {
  let fileSource = isS3 ? 'S3' : 'GitHub'
  console.log(`Downloading files from ${fileSource} to verify shasums`)
  let downloadDir = await makeTempDir()
  let filesToCheck = []
  try {
    if (!isS3) {
      await downloadFiles(urls, downloadDir)
      filesToCheck = urls.map(url => {
        let currentUrl = new URL(url)
        return path.basename(currentUrl.pathname)
      }).filter(file => file.indexOf('SHASUMS') === -1)
    } else {
      const s3VersionPath = `/atom-shell/dist/${pkgVersion}/`
      await Promise.all(urls.map(async (url) => {
        let currentUrl = new URL(url)
        let dirname = path.dirname(currentUrl.pathname)
        let filename = path.basename(currentUrl.pathname)
        let s3VersionPathIdx = dirname.indexOf(s3VersionPath)
        if (s3VersionPathIdx === -1 || dirname === s3VersionPath) {
          if (s3VersionPathIdx !== -1 && filename.indexof('SHASUMS') === -1) {
            filesToCheck.push(filename)
          }
          await downloadFiles(url, downloadDir, true)
        } else {
          let subDirectory = dirname.substr(s3VersionPathIdx + s3VersionPath.length)
          let fileDirectory = path.join(downloadDir, subDirectory)
          try {
            fs.statSync(fileDirectory)
          } catch (err) {
            fs.mkdirSync(fileDirectory)
          }
          filesToCheck.push(path.join(subDirectory, filename))
          await downloadFiles(url, fileDirectory, true)
        }
      }))
    }
  } catch (err) {
    console.log(`${fail} Error downloading files from ${fileSource}`, err)
    process.exit(1)
  }
  console.log(`${pass} Successfully downloaded the files from ${fileSource}.`)
  let checkerOpts
  if (isS3) {
    checkerOpts = { defaultTextEncoding: 'binary' }
  }

  await validateChecksums({
    algorithm: 'sha256',
    filesToCheck,
    fileDirectory: downloadDir,
    shaSumFile: 'SHASUMS256.txt',
    checkerOpts,
    fileSource
  })

  if (isS3) {
    await validateChecksums({
      algorithm: 'sha1',
      filesToCheck,
      fileDirectory: downloadDir,
      shaSumFile: 'SHASUMS.txt',
      checkerOpts,
      fileSource
    })
  }
}

async function validateChecksums (validationArgs) {
  console.log(`Validating checksums for files from ${validationArgs.fileSource} ` +
    `against ${validationArgs.shaSumFile}.`)
  let shaSumFilePath = path.join(validationArgs.fileDirectory, validationArgs.shaSumFile)
  let checker = new sumchecker.ChecksumValidator(validationArgs.algorithm,
    shaSumFilePath, validationArgs.checkerOpts)
  await checker.validate(validationArgs.fileDirectory, validationArgs.filesToCheck)
    .catch(err => {
      if (err instanceof sumchecker.ChecksumMismatchError) {
        console.error(`${fail} The checksum of ${err.filename} from ` +
          `${validationArgs.fileSource} did not match the shasum in ` +
          `${validationArgs.shaSumFile}`)
      } else if (err instanceof sumchecker.ChecksumParseError) {
        console.error(`${fail} The checksum file ${validationArgs.shaSumFile} ` +
          `from ${validationArgs.fileSource} could not be parsed.`, err)
      } else if (err instanceof sumchecker.NoChecksumFoundError) {
        console.error(`${fail} The file ${err.filename} from ` +
          `${validationArgs.fileSource} was not in the shasum file ` +
          `${validationArgs.shaSumFile}.`)
      } else {
        console.error(`${fail} Error matching files from ` +
          `${validationArgs.fileSource} shasums in ${validationArgs.shaSumFile}.`, err)
      }
      process.exit(1)
    })
  console.log(`${pass} All files from ${validationArgs.fileSource} match ` +
    `shasums defined in ${validationArgs.shaSumFile}.`)
}

async function cleanupReleaseBranch () {
  console.log(`Cleaning up release branch.`)
  let errorMessage = `Could not delete local release branch.`
  let successMessage = `Successfully deleted local release branch.`
  await callGit(['branch', '-D', 'release'], errorMessage, successMessage)
  errorMessage = `Could not delete remote release branch.`
  successMessage = `Successfully deleted remote release branch.`
  return await callGit(['push', 'origin', ':release'], errorMessage, successMessage)
}

async function callGit (args, errorMessage, successMessage) {
  let gitResult = await GitProcess.exec(args, gitDir)
  if (gitResult.exitCode === 0) {
    console.log(`${pass} ${successMessage}`)
    return true
  } else {
    console.log(`${fail} ${errorMessage} ${gitResult.stderr}`)
    process.exit(1)
  }
}

makeRelease(args.validateRelease)
