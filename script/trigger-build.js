const args = require('minimist')(process.argv.slice(2))
const prToBuild = args._[0]
const assert = require('assert')
const request = require('request')
const buildAppVeyorPRURL = 'https://windows-ci.electronjs.org/api/builds'

if (!prToBuild && !args.branch) {
  console.log(`Trigger CI to build.
  Usage: trigger-build.js [--project=electron|libchromiumcontent] [--branch=BRANCHNAME] [PR_NUMBER]
  `)
  process.exit(1)
}

const projectSlug = args.project || 'electron'

assert(process.env.APPVEYOR_TOKEN, 'APPVEYOR_TOKEN not found in environment')
let buildOpts = {
  accountName: 'AppVeyor',
  projectSlug
}
if (prToBuild) {
  buildOpts.pullRequestId = prToBuild
}
if (args.branch) {
  buildOpts.branch = args.branch
}

request.post(buildAppVeyorPRURL, {
  'auth': {
    'bearer': process.env.APPVEYOR_TOKEN
  },
  headers: {
    'Content-Type': 'application/json'
  },
  form: buildOpts
}, (err, res, body) => {
  if (!err && res.statusCode >= 200 && res.statusCode < 300) {
    const build = JSON.parse(body)
    console.log('PR request successful', build)
  } else {
    console.log('Error: ', `(status ${res.statusCode})`, err || JSON.parse(res.body))
  }
})
