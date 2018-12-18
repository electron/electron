if (!process.env.CI) require('dotenv-safe').load()

const GitHub = require('github')
const github = new GitHub()
github.authenticate({type: 'token', token: process.env.ELECTRON_GITHUB_TOKEN})

if (process.argv.length < 6) {
  console.log('Usage: upload-to-github filePath fileName releaseId')
  process.exit(1)
}
let filePath = process.argv[2]
let fileName = process.argv[3]
let releaseId = process.argv[4]
let releaseVersion = process.argv[5]

const targetRepo = releaseVersion.indexOf('nightly') > 0 ? 'nightlies' : 'electron'

let githubOpts = {
  owner: 'electron',
  repo: targetRepo,
  id: releaseId,
  filePath: filePath,
  name: fileName
}

let retry = 0

function uploadToGitHub () {
  github.repos.uploadAsset(githubOpts).then(() => {
    console.log(`Successfully uploaded ${fileName} to GitHub.`)
    process.exit()
  }).catch((err) => {
    if (retry < 4) {
      console.log(`Error uploading ${fileName} to GitHub, will retry.  Error was:`, err)
      retry++
      github.repos.getRelease(githubOpts).then(release => {
        console.log('Got list of assets for existing release:')
        console.log(JSON.stringify(release.data.assets, null, '  '))
        let existingAssets = release.data.assets.filter(asset => asset.name === fileName)
        if (existingAssets.length > 0) {
          console.log(`${fileName} already exists; will delete before retrying upload.`)
          github.repos.deleteAsset({
            owner: 'electron',
            repo: targetRepo,
            id: existingAssets[0].id
          }).catch((deleteErr) => {
            console.log(`Failed to delete existing asset ${fileName}.  Error was:`, deleteErr)
          }).then(uploadToGitHub)
        } else {
          console.log(`Current asset ${fileName} not found in existing assets; retrying upload.`)
          uploadToGitHub()
        }
      }).catch((getReleaseErr) => {
        console.log(`Fatal: Unable to get current release assets via getRelease!  Error was:`, getReleaseErr)
      })
    } else {
      console.log(`Error retrying uploading ${fileName} to GitHub:`, err)
      process.exitCode = 1
    }
  })
}

uploadToGitHub()
