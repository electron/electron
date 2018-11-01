const temp = require('temp')
const fs = require('fs')
const path = require('path')
const childProcess = require('child_process')
const GitHubApi = require('github')
const {GitProcess} = require('dugite')
const request = require('request')
const semver = require('semver')
const rootPackageJson = require('../package.json')

if (!process.env.ELECTRON_NPM_OTP) {
  console.error('Please set ELECTRON_NPM_OTP')
  process.exit(1)
}

const github = new GitHubApi({
  // debug: true,
  headers: { 'User-Agent': 'electron-npm-publisher' },
  followRedirects: false
})

let tempDir
temp.track()   // track and cleanup files at exit

const files = [
  'cli.js',
  'index.js',
  'install.js',
  'package.json',
  'README.md',
  'LICENSE'
]

const jsonFields = [
  'name',
  'version',
  'repository',
  'description',
  'license',
  'author',
  'keywords'
]

let npmTag = ''

new Promise((resolve, reject) => {
  temp.mkdir('electron-npm', (err, dirPath) => {
    if (err) {
      reject(err)
    } else {
      resolve(dirPath)
    }
  })
})
.then((dirPath) => {
  tempDir = dirPath
  // copy files from `/npm` to temp directory
  files.forEach((name) => {
    const noThirdSegment = name === 'README.md' || name === 'LICENSE'
    fs.writeFileSync(
      path.join(tempDir, name),
      fs.readFileSync(path.join(__dirname, '..', noThirdSegment ? '' : 'npm', name))
    )
  })
  // copy from root package.json to temp/package.json
  const packageJson = require(path.join(tempDir, 'package.json'))
  jsonFields.forEach((fieldName) => {
    packageJson[fieldName] = rootPackageJson[fieldName]
  })
  fs.writeFileSync(
    path.join(tempDir, 'package.json'),
    JSON.stringify(packageJson, null, 2)
  )

  return github.repos.getReleases({
    owner: 'electron',
    repo: rootPackageJson.version.indexOf('nightly') > 0 ? 'nightlies' : 'electron'
  })
})
.then((releases) => {
  // download electron.d.ts from release
  const release = releases.data.find(
    (release) => release.tag_name === `v${rootPackageJson.version}`
  )
  if (!release) {
    throw new Error(`cannot find release with tag v${rootPackageJson.version}`)
  }
  return release
})
.then((release) => {
  const tsdAsset = release.assets.find((asset) => asset.name === 'electron.d.ts')
  if (!tsdAsset) {
    throw new Error(`cannot find electron.d.ts from v${rootPackageJson.version} release assets`)
  }
  return new Promise((resolve, reject) => {
    request.get({
      url: tsdAsset.url,
      headers: {
        'accept': 'application/octet-stream',
        'user-agent': 'electron-npm-publisher'
      }
    }, (err, response, body) => {
      if (err || response.statusCode !== 200) {
        reject(err || new Error('Cannot download electron.d.ts'))
      } else {
        fs.writeFileSync(path.join(tempDir, 'electron.d.ts'), body)
        resolve(release)
      }
    })
  })
})
.then(async (release) => {
  const currentBranch = await getCurrentBranch()

  if (release.tag_name.indexOf('nightly') > 0) {
    if (currentBranch === 'master') {
      npmTag = 'nightly'
    } else {
      npmTag = `nightly-${currentBranch}`
    }
  } else {
    if (currentBranch === 'master') {
      // This should never happen, master releases should be nightly releases
      // this is here just-in-case
      npmTag = 'master'
    } else if (!release.prerelease) {
      // Tag the release with a `2-0-x` style tag
      npmTag = currentBranch
    } else {
      // Tag the release with a `beta-3-0-x` style tag
      npmTag = `beta-${currentBranch}`
    }
  }
})
.then(() => childProcess.execSync('npm pack', { cwd: tempDir }))
.then(() => {
  // test that the package can install electron prebuilt from github release
  const tarballPath = path.join(tempDir, `${rootPackageJson.name}-${rootPackageJson.version}.tgz`)
  return new Promise((resolve, reject) => {
    childProcess.execSync(`npm install ${tarballPath} --force --silent`, {
      env: Object.assign({}, process.env, { electron_config_cache: tempDir }),
      cwd: tempDir
    })
    resolve(tarballPath)
  })
})
.then((tarballPath) => childProcess.execSync(`npm publish ${tarballPath} --tag ${npmTag} --otp=${process.env.ELECTRON_NPM_OTP}`))
.then(() => {
  const currentTags = JSON.parse(childProcess.execSync('npm show electron dist-tags --json').toString())
  const localVersion = rootPackageJson.version
  const parsedLocalVersion = semver.parse(localVersion)
  if (parsedLocalVersion.prerelease.length === 0 &&
        semver.gt(localVersion, currentTags.latest)) {
    childProcess.execSync(`npm dist-tag add electron@${localVersion} latest --otp=${process.env.ELECTRON_NPM_OTP}`)
  }
  if (parsedLocalVersion.prerelease[0] === 'beta' &&
        semver.gt(localVersion, currentTags.beta)) {
    childProcess.execSync(`npm dist-tag add electron@${localVersion} beta --otp=${process.env.ELECTRON_NPM_OTP}`)
  }
})
.catch((err) => {
  console.error(`Error: ${err}`)
  process.exit(1)
})

async function getCurrentBranch () {
  const gitDir = path.resolve(__dirname, '..')
  console.log(`Determining current git branch`)
  let gitArgs = ['rev-parse', '--abbrev-ref', 'HEAD']
  let branchDetails = await GitProcess.exec(gitArgs, gitDir)
  if (branchDetails.exitCode === 0) {
    let currentBranch = branchDetails.stdout.trim()
    console.log(`Successfully determined current git branch is ` +
      `${currentBranch}`)
    return currentBranch
  } else {
    let error = GitProcess.parseError(branchDetails.stderr)
    console.log(`Could not get details for the current branch,
      error was ${branchDetails.stderr}`, error)
    process.exit(1)
  }
}
