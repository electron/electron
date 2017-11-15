const args = require('minimist')(process.argv.slice(2))
const assert = require('assert')
const request = require('request')

const ciJobs = [
  'electron-linux-arm64',
  'electron-linux-ia32',
  'electron-linux-x64',
  'electron-linux-arm',
  'electron-osx-x64',
  'electron-mas-x64'
]

const CIcall = (buildUrl, targetBranch, job) => {
  console.log(`Triggering CircleCI to run build job: ${job} on branch: ${targetBranch} with release flag.`)

  request({
    method: 'POST',
    url: buildUrl,
    headers: {
      'Content-Type': 'application/json',
      'Accept': 'application/json'
    },
    body: JSON.stringify({
      'build_parameters': {
        'RUN_RELEASE_BUILD': 'true',
        'CIRCLE_JOB': job
      }
    })
  }, (err, res, body) => {
    if (!err && res.statusCode >= 200 && res.statusCode < 300) {
      const build = JSON.parse(body)
      console.log(`Check ${build.build_url} for status. (${job})`)
    } else {
      console.log('Error: ', `(status ${res.statusCode})`, err || JSON.parse(res.body), job)
    }
  })
}

if (args._.length < 1) {
  console.log(`Trigger Circle CI to build release builds of electron.
  Usage: ci-release-build.js [--job=CI_JOB_NAME] TARGET_BRANCH
  `)
  process.exit(0)
}

assert(process.env.CIRCLE_TOKEN, 'CIRCLE_TOKEN not found in environment')

const targetBranch = args._[0]
const job = args['job']
const circleBuildUrl = `https://circleci.com/api/v1.1/project/github/electron/electron/tree/${targetBranch}?circle-token=${process.env.CIRCLE_TOKEN}`

if (job) {
  assert(ciJobs.includes(job), `Unknown CI job name: ${job}.`)
  CIcall(circleBuildUrl, targetBranch, job)
} else {
  ciJobs.forEach((job) => CIcall(circleBuildUrl, targetBranch, job))
}
