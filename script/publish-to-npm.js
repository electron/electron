require('dotenv-safe').load()

const temp = require('temp'),
      fs   = require('fs'),
      util = require('util'),
      path = require('path'),
      childProcess = require('child_process'),
      GitHubApi = require('github'),
      request = require('request'),
      rootPackageJson = require('../package.json'),
      assert = require('assert')

const github = new GitHubApi({
  // debug: true,
  headers: { 'User-Agent': 'electron-npm-publisher' },
  followRedirects: false,
})
github.authenticate({
  type: 'token',
  token: process.env.GITHUB_TOKEN
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
      fs.readFileSync(path.join(__dirname, '..', 'npm', name))
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
    repo: 'electron',
  })
})
.then((releases) => {
  // download electron.d.ts from draft release
  const draftRelease = releases.data.find(
    (release) => release.draft && release.tag_name === `v${rootPackageJson.version}`
    // (release) => release.draft && release.tag_name === `test`
  )
  if (!draftRelease) {
    throw `cannot find release with tag v${rootPackageJson.version}`
  }
  return draftRelease.assets.find((asset) => asset.name === 'electron.d.ts')
})
.then((tsdAsset) => {
  if (!tsdAsset) {
    throw `cannot find electron.d.ts from v${rootPackageJson.version} draft release assets`
  }
  return new Promise((resolve, reject) => {
    request.get({
      url: tsdAsset.url,
      headers: {
        'accept': 'application/octet-stream',
        'user-agent': 'electron-npm-publisher',
        Authorization: `token ${process.env.GITHUB_TOKEN}`
      }
    }, (err, response, body) => {
      if (err) {
        reject(err)
      } else {
        fs.writeFileSync(path.join(tempDir, 'electron.d.ts'), body)
        resolve()
      }
    })
  })
})
.then(() => childProcess.execSync('npm pack', { cwd: tempDir }))
.then(() => {
  // test that the package can install electron prebuilt from /dist
  const distDir = path.join(__dirname, '..', 'dist')
  const tarballPath = path.join(tempDir, `electron-${rootPackageJson.version}.tgz`)
  return new Promise((resolve, reject) => {
    childProcess.execSync(`npm install ${tarballPath} --force --silent`, {
      env: Object.assign({}, process.env, { electron_config_cache: distDir }),
      cwd: tempDir,
    })
    const checkVersion = childProcess.exec(`${path.join(tempDir, 'node_modules', '.bin', 'electron')} -v`)
    checkVersion.stdout.on('data', (data) => {
      assert.strictEqual(data.trim(), `v${rootPackageJson.version}`)
      resolve(tarballPath)
    })
  })
})
.then((tarballPath) => childProcess.execSync(`npm publish ${tarballPath}`))
.catch((err) => console.error(`Error: ${err}`))
