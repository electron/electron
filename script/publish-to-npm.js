const temp = require('temp')
const fs = require('fs')
const path = require('path')
const childProcess = require('child_process')
const GitHubApi = require('github')
const request = require('request')
const assert = require('assert')
const rootPackageJson = require('../package.json')

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
  'README.md'
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
    fs.writeFileSync(
      path.join(tempDir, name),
      fs.readFileSync(path.join(__dirname, '..', name === 'README.md' ? '' : 'npm', name))
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
    repo: 'electron'
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
.then((release) => {
  npmTag = release.prerelease ? 'beta' : 'latest'
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
    const checkVersion = childProcess.execSync(`${path.join(tempDir, 'node_modules', '.bin', 'electron')} -v`)
    assert.ok((`v${rootPackageJson.version}`.indexOf(checkVersion.toString().trim()) === 0), `Version is correct`)
    resolve(tarballPath)
  })
})
.then((tarballPath) => childProcess.execSync(`npm publish ${tarballPath} --tag ${npmTag}`))
.catch((err) => {
  console.error(`Error: ${err}`)
  process.exit(1)
})
