require('dotenv-safe').load()

const assert = require('assert')
const request = require('request')
const buildAppVeyorURL = 'https://ci.appveyor.com/api/builds'

const appVeyorJobs = {
  'electron-x64': 'electron-x64-release',
  'electron-ia32': 'electron-ia32-release'
}

const circleCIJobs = [
  'electron-linux-arm',
  'electron-linux-arm64',
  'electron-linux-ia32',
  'electron-linux-x64',
  'electron-osx-x64',
  'electron-mas-x64'
]

async function makeRequest (requestOptions, parseResponse) {
  return new Promise((resolve, reject) => {
    request(requestOptions, (err, res, body) => {
      if (!err && res.statusCode >= 200 && res.statusCode < 300) {
        if (parseResponse) {
          const build = JSON.parse(body)
          resolve(build)
        } else {
          resolve(body)
        }
      } else {
        console.error('Error occurred while requesting:', requestOptions.url)
        if (parseResponse) {
          try {
            console.log('Error: ', `(status ${res.statusCode})`, err || JSON.parse(res.body), requestOptions)
          } catch (err) {
            console.log('Error: ', `(status ${res.statusCode})`, err || res.body, requestOptions)
          }
        } else {
          console.log('Error: ', `(status ${res.statusCode})`, err || res.body, requestOptions)
        }
        reject(err)
      }
    })
  })
}

async function circleCIcall (buildUrl, targetBranch, job, options) {
  console.log(`Triggering CircleCI to run build job: ${job} on branch: ${targetBranch} with release flag.`)
  let buildRequest = {
    'build_parameters': {
      'CIRCLE_JOB': job
    }
  }

  if (options.ghRelease) {
    buildRequest.build_parameters.ELECTRON_RELEASE = 1
  } else {
    buildRequest.build_parameters.RUN_RELEASE_BUILD = 'true'
  }

  if (options.automaticRelease) {
    buildRequest.build_parameters.AUTO_RELEASE = 'true'
  }

  let circleResponse = await makeRequest({
    method: 'POST',
    url: buildUrl,
    headers: {
      'Content-Type': 'application/json',
      'Accept': 'application/json'
    },
    body: JSON.stringify(buildRequest)
  }, true).catch(err => {
    console.log('Error calling CircleCI:', err)
  })
  console.log(`CircleCI release build request for ${job} successful.  Check ${circleResponse.build_url} for status.`)
}

function buildAppVeyor (targetBranch, options) {
  const validJobs = Object.keys(appVeyorJobs)
  if (options.job) {
    assert(validJobs.includes(options.job), `Unknown AppVeyor CI job name: ${options.job}.  Valid values are: ${validJobs}.`)
    callAppVeyor(targetBranch, options.job, options)
  } else {
    validJobs.forEach((job) => callAppVeyor(targetBranch, job, options))
  }
}

async function callAppVeyor (targetBranch, job, options) {
  console.log(`Triggering AppVeyor to run build job: ${job} on branch: ${targetBranch} with release flag.`)
  let environmentVariables = {}

  if (options.ghRelease) {
    environmentVariables.ELECTRON_RELEASE = 1
  } else {
    environmentVariables.RUN_RELEASE_BUILD = 'true'
  }

  if (options.automaticRelease) {
    environmentVariables.AUTO_RELEASE = 'true'
  }

  const requestOpts = {
    url: buildAppVeyorURL,
    auth: {
      bearer: process.env.APPVEYOR_CLOUD_TOKEN
    },
    headers: {
      'Content-Type': 'application/json'
    },
    body: JSON.stringify({
      accountName: 'electron-bot',
      projectSlug: appVeyorJobs[job],
      branch: targetBranch,
      environmentVariables
    }),
    method: 'POST'
  }
  let appVeyorResponse = await makeRequest(requestOpts, true).catch(err => {
    console.log('Error calling AppVeyor:', err)
  })
  const buildUrl = `https://ci.appveyor.com/project/electron-bot/${appVeyorJobs[job]}/build/${appVeyorResponse.version}`
  console.log(`AppVeyor release build request for ${job} successful.  Check build status at ${buildUrl}`)
}

function buildCircleCI (targetBranch, options) {
  const circleBuildUrl = `https://circleci.com/api/v1.1/project/github/electron/electron/tree/${targetBranch}?circle-token=${process.env.CIRCLE_TOKEN}`
  if (options.job) {
    assert(circleCIJobs.includes(options.job), `Unknown CircleCI job name: ${options.job}. Valid values are: ${circleCIJobs}.`)
    circleCIcall(circleBuildUrl, targetBranch, options.job, options)
  } else {
    circleCIJobs.forEach((job) => circleCIcall(circleBuildUrl, targetBranch, job, options))
  }
}

function runRelease (targetBranch, options) {
  if (options.ci) {
    switch (options.ci) {
      case 'CircleCI': {
        buildCircleCI(targetBranch, options)
        break
      }
      case 'AppVeyor': {
        buildAppVeyor(targetBranch, options)
        break
      }
      default: {
        console.log(`Error! Unknown CI: ${options.ci}.`)
        process.exit(1)
      }
    }
  } else {
    buildCircleCI(targetBranch, options)
    buildAppVeyor(targetBranch, options)
  }
}

module.exports = runRelease

if (require.main === module) {
  const args = require('minimist')(process.argv.slice(2), {
    boolean: ['ghRelease', 'automaticRelease']
  })
  const targetBranch = args._[0]
  if (args._.length < 1) {
    console.log(`Trigger CI to build release builds of electron.
    Usage: ci-release-build.js [--job=CI_JOB_NAME] [--ci=CircleCI|AppVeyor] [--ghRelease] [--automaticRelease] TARGET_BRANCH
    `)
    process.exit(0)
  }
  runRelease(targetBranch, args)
}
