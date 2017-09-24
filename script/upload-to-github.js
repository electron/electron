const GitHub = require('github')
const github = new GitHub()
github.authenticate({type: 'token', token: process.env.ELECTRON_GITHUB_TOKEN})

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
github.repos.uploadAsset(githubOpts).then(() => {
  process.exit()
}).catch((err) => {
  console.log(`Error uploading ${fileName} to GitHub:`, err)
  process.exitCode = 1
})
