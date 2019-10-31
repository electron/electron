if (!process.env.CI) require('dotenv-safe').load()

const assert = require('assert')
const request = require('request')
const buildAppVeyorURL = 'https://ci.appveyor.com/api/builds'
const vstsURL = 'https://github.visualstudio.com/electron/_apis/build'

const appVeyorJobs = {
  'electron-x64': 'electron-x64-release',
  'electron-ia32': 'electron-ia32-release'
}

const circleCIJobs = [
  'linux-arm-publish',
  'linux-arm64-publish',
  'linux-ia32-publish',
  'linux-x64-publish',
  'mas-publish',
  'osx-publish'
]

const vstsArmJobs = [
  'electron-arm-testing',
  'electron-arm64-testing'
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
  const buildRequest = {
    'build_parameters': {
      'CIRCLE_JOB': job
    }
  }

  if (!options.ghRelease) {
    buildRequest.build_parameters.UPLOAD_TO_S3 = 1
  }

  const circleResponse = await makeRequest({
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
  const environmentVariables = {
    ELECTRON_RELEASE: 1
  }

  if (!options.ghRelease) {
    environmentVariables.UPLOAD_TO_S3 = 1
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
  const appVeyorResponse = await makeRequest(requestOpts, true).catch(err => {
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

async function buildVSTS (targetBranch, options) {
  if (options.armTest) {
    assert(vstsArmJobs.includes(options.job), `Unknown VSTS CI arm test job name: ${options.job}. Valid values are: ${vstsArmJobs}.`)
  }

  console.log(`Triggering VSTS to run build on branch: ${targetBranch} with release flag.`)
  const environmentVariables = {
    ELECTRON_RELEASE: 1
  }

  if (options.armTest) {
    environmentVariables.CIRCLE_BUILD_NUM = options.circleBuildNum
  } else {
    if (!options.ghRelease) {
      environmentVariables.UPLOAD_TO_S3 = 1
    }
  }

  const requestOpts = {
    url: `${vstsURL}/definitions?api-version=4.1`,
    auth: {
      user: '',
      password: process.env.VSTS_TOKEN
    },
    headers: {
      'Content-Type': 'application/json'
    }
  }
  const vstsResponse = await makeRequest(requestOpts, true).catch(err => {
    console.log('Error calling VSTS to get build definitions:', err)
  })
  const buildsToRun = vstsResponse.value.filter(build => build.name === options.job)
  buildsToRun.forEach((build) => callVSTSBuild(build, targetBranch, environmentVariables))
}

async function callVSTSBuild (build, targetBranch, environmentVariables) {
  const buildBody = {
    definition: build,
    sourceBranch: targetBranch,
    priority: 'high'
  }
  if (Object.keys(environmentVariables).length !== 0) {
    buildBody.parameters = JSON.stringify(environmentVariables)
  }
  const requestOpts = {
    url: `${vstsURL}/builds?api-version=4.1`,
    auth: {
      user: '',
      password: process.env.VSTS_TOKEN
    },
    headers: {
      'Content-Type': 'application/json'
    },
    body: JSON.stringify(buildBody),
    method: 'POST'
  }
  const vstsResponse = await makeRequest(requestOpts, true).catch(err => {
    console.log(`Error calling VSTS for job ${build.name}`, err)
  })
  console.log(`VSTS release build request for ${build.name} successful. Check ${vstsResponse._links.web.href} for status.`)
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
      case 'VSTS': {
        buildVSTS(targetBranch, options)
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
    boolean: ['ghRelease', 'armTest']
  })
  const targetBranch = args._[0]
  if (args._.length < 1) {
    console.log(`Trigger CI to build release builds of electron.
    Usage: ci-release-build.js [--job=CI_JOB_NAME] [--ci=CircleCI|AppVeyor|VSTS]
    [--ghRelease] [--armTest] [--circleBuildNum=xxx] TARGET_BRANCH
    `)
    process.exit(0)
  }
  runRelease(targetBranch, args)
}
