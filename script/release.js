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

const github = new GitHub({
  followRedirects: false
})
github.authenticate({type: 'token', token: process.env.ELECTRON_GITHUB_TOKEN})

async function getDraftRelease (version, skipValidation) {
  let releaseInfo = await github.repos.getReleases({owner: 'electron', repo: 'electron'})
  let drafts
  let versionToCheck
  if (version) {
    versionToCheck = version
  } else {
    versionToCheck = pkgVersion
  }
  drafts = releaseInfo.data
    .filter(release => release.tag_name === versionToCheck &&
      release.draft === true)
  const draft = drafts[0]
  if (!skipValidation) {
    failureCount = 0
    check(drafts.length === 1, 'one draft exists', true)
    if (versionToCheck.indexOf('beta') > -1) {
      check(draft.prerelease, 'draft is a prerelease')
    }
    check(draft.body.length > 50 && !draft.body.includes('(placeholder)'), 'draft has release notes')
    check((failureCount === 0), `Draft release looks good to go.`, true)
  }
  return draft
}

async function validateReleaseAssets (release, validatingRelease) {
  const requiredAssets = assetsForVersion(release.tag_name, validatingRelease).sort()
  const extantAssets = release.assets.map(asset => asset.name).sort()
  const downloadUrls = release.assets.map(asset => asset.browser_download_url).sort()

  failureCount = 0
  requiredAssets.forEach(asset => {
    check(extantAssets.includes(asset), asset)
  })
  check((failureCount === 0), `All required GitHub assets exist for release`, true)

  if (!validatingRelease || !release.draft) {
    if (release.draft) {
      await verifyAssets(release)
    } else {
      await verifyShasums(downloadUrls)
        .catch(err => {
          console.log(`${fail} error verifyingShasums`, err)
        })
    }
    const s3Urls = s3UrlsForVersion(release.tag_name)
    await verifyShasums(s3Urls, true)
  }
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

function assetsForVersion (version, validatingRelease) {
  const patterns = [
    `electron-${version}-darwin-x64-dsym.zip`,
    `electron-${version}-darwin-x64-symbols.zip`,
    `electron-${version}-darwin-x64.zip`,
    `electron-${version}-linux-arm-symbols.zip`,
    `electron-${version}-linux-arm.zip`,
    `electron-${version}-linux-arm64-symbols.zip`,
    `electron-${version}-linux-arm64.zip`,
    `electron-${version}-linux-armv7l-symbols.zip`,
    `electron-${version}-linux-armv7l.zip`,
    `electron-${version}-linux-ia32-symbols.zip`,
    `electron-${version}-linux-ia32.zip`,
//    `electron-${version}-linux-mips64el.zip`,
    `electron-${version}-linux-x64-symbols.zip`,
    `electron-${version}-linux-x64.zip`,
    `electron-${version}-mas-x64-dsym.zip`,
    `electron-${version}-mas-x64-symbols.zip`,
    `electron-${version}-mas-x64.zip`,
    `electron-${version}-win32-ia32-pdb.zip`,
    `electron-${version}-win32-ia32-symbols.zip`,
    `electron-${version}-win32-ia32.zip`,
    `electron-${version}-win32-x64-pdb.zip`,
    `electron-${version}-win32-x64-symbols.zip`,
    `electron-${version}-win32-x64.zip`,
    `electron-api.json`,
    `electron.d.ts`,
    `ffmpeg-${version}-darwin-x64.zip`,
    `ffmpeg-${version}-linux-arm.zip`,
    `ffmpeg-${version}-linux-arm64.zip`,
    `ffmpeg-${version}-linux-armv7l.zip`,
    `ffmpeg-${version}-linux-ia32.zip`,
//    `ffmpeg-${version}-linux-mips64el.zip`,
    `ffmpeg-${version}-linux-x64.zip`,
    `ffmpeg-${version}-mas-x64.zip`,
    `ffmpeg-${version}-win32-ia32.zip`,
    `ffmpeg-${version}-win32-x64.zip`
  ]
  if (!validatingRelease) {
    patterns.push('SHASUMS256.txt')
  }
  return patterns
}

function s3UrlsForVersion (version) {
  const bucket = `https://gh-contractor-zcbenz.s3.amazonaws.com/`
  const patterns = [
    `${bucket}atom-shell/dist/${version}/iojs-${version}-headers.tar.gz`,
    `${bucket}atom-shell/dist/${version}/iojs-${version}.tar.gz`,
    `${bucket}atom-shell/dist/${version}/node-${version}.tar.gz`,
    `${bucket}atom-shell/dist/${version}/node.lib`,
    `${bucket}atom-shell/dist/${version}/win-x64/iojs.lib`,
    `${bucket}atom-shell/dist/${version}/win-x86/iojs.lib`,
    `${bucket}atom-shell/dist/${version}/x64/node.lib`,
    `${bucket}atom-shell/dist/${version}/SHASUMS.txt`,
    `${bucket}atom-shell/dist/${version}/SHASUMS256.txt`,
    `${bucket}atom-shell/dist/index.json`
  ]
  return patterns
}

function checkVersion () {
  console.log(`Verifying that app version matches package version ${pkgVersion}.`)
  let startScript = path.join(__dirname, 'start.py')
  let scriptArgs = ['--version']
  if (args.automaticRelease) {
    scriptArgs.unshift('-R')
  }
  let appVersion = runScript(startScript, scriptArgs).trim()
  check((pkgVersion.indexOf(appVersion) === 0), `App version ${appVersion} matches ` +
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
  let scriptArgs = []
  if (args.automaticRelease) {
    scriptArgs.push('-R')
  }
  runScript(scriptPath, scriptArgs)
  console.log(`${pass} Done uploading index.json to S3.`)
}

async function createReleaseShasums (release) {
  let fileName = 'SHASUMS256.txt'
  let existingAssets = release.assets.filter(asset => asset.name === fileName)
  if (existingAssets.length > 0) {
    console.log(`${fileName} already exists on GitHub; deleting before creating new file.`)
    await github.repos.deleteAsset({
      owner: 'electron',
      repo: 'electron',
      id: existingAssets[0].id
    }).catch(err => {
      console.log(`${fail} Error deleting ${fileName} on GitHub:`, err)
    })
  }
  console.log(`Creating and uploading the release ${fileName}.`)
  let scriptPath = path.join(__dirname, 'merge-electron-checksums.py')
  let checksums = runScript(scriptPath, ['-v', pkgVersion])
  console.log(`${pass} Generated release SHASUMS.`)
  let filePath = await saveShaSumFile(checksums, fileName)
  console.log(`${pass} Created ${fileName} file.`)
  await uploadShasumFile(filePath, fileName, release)
  console.log(`${pass} Successfully uploaded ${fileName} to GitHub.`)
}

async function uploadShasumFile (filePath, fileName, release) {
  let githubOpts = {
    owner: 'electron',
    repo: 'electron',
    id: release.id,
    filePath,
    name: fileName
  }
  return github.repos.uploadAsset(githubOpts)
    .catch(err => {
      console.log(`${fail} Error uploading ${filePath} to GitHub:`, err)
      process.exit(1)
    })
}

function saveShaSumFile (checksums, fileName) {
  return new Promise((resolve, reject) => {
    temp.open(fileName, (err, info) => {
      if (err) {
        console.log(`${fail} Could not create ${fileName} file`)
        process.exit(1)
      } else {
        fs.writeFileSync(info.fd, checksums)
        fs.close(info.fd, (err) => {
          if (err) {
            console.log(`${fail} Could close ${fileName} file`)
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
    tag_name: release.tag_name,
    draft: false
  }
  return github.repos.editRelease(githubOpts)
    .catch(err => {
      console.log(`${fail} Error publishing release:`, err)
      process.exit(1)
    })
}

async function makeRelease (releaseToValidate) {
  if (releaseToValidate) {
    if (releaseToValidate === true) {
      releaseToValidate = pkgVersion
    } else {
      console.log('Release to validate !=== true')
    }
    console.log(`Validating release ${releaseToValidate}`)
    let release = await getDraftRelease(releaseToValidate)
    await validateReleaseAssets(release, true)
  } else {
    checkVersion()
    let draftRelease = await getDraftRelease()
    uploadNodeShasums()
    uploadIndexJson()
    await createReleaseShasums(draftRelease)
    // Fetch latest version of release before verifying
    draftRelease = await getDraftRelease(pkgVersion, true)
    await validateReleaseAssets(draftRelease)
    await tagLibCC()
    await publishRelease(draftRelease)
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

async function verifyAssets (release) {
  let downloadDir = await makeTempDir()
  let githubOpts = {
    owner: 'electron',
    repo: 'electron',
    headers: {
      Accept: 'application/octet-stream'
    }
  }
  console.log(`Downloading files from GitHub to verify shasums`)
  let shaSumFile = 'SHASUMS256.txt'
  let filesToCheck = await Promise.all(release.assets.map(async (asset) => {
    githubOpts.id = asset.id
    let assetDetails = await github.repos.getAsset(githubOpts)
    await downloadFiles(assetDetails.meta.location, downloadDir, false, asset.name)
    return asset.name
  })).catch(err => {
    console.log(`${fail} Error downloading files from GitHub`, err)
    process.exit(1)
  })
  filesToCheck = filesToCheck.filter(fileName => fileName !== shaSumFile)
  let checkerOpts
  await validateChecksums({
    algorithm: 'sha256',
    filesToCheck,
    fileDirectory: downloadDir,
    shaSumFile,
    checkerOpts,
    fileSource: 'GitHub'
  })
}

function downloadFiles (urls, directory, quiet, targetName) {
  return new Promise((resolve, reject) => {
    let nuggetOpts = {
      dir: directory
    }
    if (quiet) {
      nuggetOpts.quiet = quiet
    }
    if (targetName) {
      nuggetOpts.target = targetName
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

async function tagLibCC () {
  const tag = `electron-${pkg.version}`
  const libccDir = path.join(path.resolve(__dirname, '..'), 'vendor', 'libchromiumcontent')
  console.log(`Tagging release ${tag}.`)
  let tagDetails = await GitProcess.exec([ 'tag', '-a', '-m', tag, tag ], libccDir)
  if (tagDetails.exitCode === 0) {
    let pushDetails = await GitProcess.exec(['push', '--tags'], libccDir)
    if (pushDetails.exitCode === 0) {
      console.log(`${pass} Successfully tagged libchromiumcontent with ${tag}.`)
    } else {
      console.log(`${fail} Error pushing libchromiumcontent tag ${tag}: ` +
        `${pushDetails.stderr}`)
    }
  } else {
    console.log(`${fail} Error tagging libchromiumcontent with ${tag}: ` +
      `${tagDetails.stderr}`)
  }
}

makeRelease(args.validateRelease)
