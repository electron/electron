if (!process.env.CI) require('dotenv-safe').load()

const GitHub = require('github')
const github = new GitHub()

if (process.argv.length < 3) {
  console.log('Usage: find-release version')
  process.exit(1)
}

const version = process.argv[2]

async function findRelease () {
  github.authenticate({type: 'token', token: process.env.ELECTRON_GITHUB_TOKEN})
  let releases = await github.repos.getReleases({
    owner: 'electron',
    repo: version.indexOf('nightly') > 0 ? 'nightlies' : 'electron'
  })
  let targetRelease = releases.data.find(release => {
    return release.tag_name === version
  })
  let returnObject = {}

  if (targetRelease) {
    returnObject = {
      id: targetRelease.id,
      draft: targetRelease.draft,
      exists: true
    }
  } else {
    returnObject = {
      exists: false,
      draft: false
    }
  }
  console.log(JSON.stringify(returnObject))
}

findRelease()
