const args = require('minimist')(process.argv.slice(2))
const assert = require('assert')
const request = require('request')

const ciJobs = [
  'electron-linux-arm64',
  'electron-linux-ia32',
  'electron-linux-x64',
  'electron-linux-arm'
]

if (args._.length < 1) {
  console.log(`Trigger Circle CI to build release builds of electron.
  Usage: ci-release-build.js [--branch=TARGET_BRANCH] CI_JOB_NAME
  `)
  process.exit(1)
}

const targetBranch = args['branch'] || 'master'
const ciJob = args._[0]
assert(ciJobs.includes(ciJob), `Unknown ci job name: ${ciJob}.`)
assert(process.env.CIRCLE_TOKEN, 'CIRCLE_TOKEN not found in environment')


const circleBuildURL = `https://circleci.com/api/v1.1/project/github/electron/electron/tree/${targetBranch}?circle-token=${process.env.CIRCLE_TOKEN}`

console.log(`Triggering CircleCI to run build job: ${ciJob} against branch: ${targetBranch} with release flag.`)

request({
  method: 'POST',
  url: circleBuildURL,
  headers: {
    'Content-Type': 'application/json',
    'Accept': 'application/json'
  },
  body: JSON.stringify({
    'build_parameters': {
      'RUN_RELEASE_BUILD': 'true',
      'CIRCLE_JOB': ciJob
    }
  })
}, (err, res, body) => {
  if (!err && res.statusCode == 200) {
    const build = JSON.parse(body)
    console.log(`check ${build.build_url} for status`)
  } else {
    console.log('error', err)
  }
})
