const GitHub = require('github')
const github = new GitHub()
github.authenticate({type: 'token', token: process.env.ELECTRON_GITHUB_TOKEN})

if (process.argv.length < 5) {
  console.log('Usage: upload-to-github filePath fileName releaseId')
  process.exit(1)
}
let filePath = process.argv[2]
let fileName = process.argv[3]
let releaseId = process.argv[4]

let githubOpts = {
  owner: 'electron',
  repo: 'electron',
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
      uploadToGitHub()
    } else {
      console.log(`Error retrying uploading ${fileName} to GitHub:`, err)
      process.exitCode = 1
    }
  })
}

uploadToGitHub()
