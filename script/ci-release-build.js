const assert = require('assert')
const request = require('request')
const buildAppVeyorURL = 'https://windows-ci.electronjs.org/api/builds'
const vstsURL = 'https://github.visualstudio.com/electron/_apis/build'

const circleCIJobs = [
  'electron-linux-arm',
  'electron-linux-arm64',
  'electron-linux-ia32',
//  'electron-linux-mips64el',
  'electron-linux-x64'
]

const vstsJobs = [
  'electron-release-mas-x64',
  'electron-release-osx-x64'
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
        if (parseResponse) {
          console.log('Error: ', `(status ${res.statusCode})`, err || JSON.parse(res.body), requestOptions)
        } else {
          console.log('Error: ', `(status ${res.statusCode})`, err || res.body, requestOptions)
        }
        reject(err)
      }
    })
  })
}

async function circleCIcall (buildUrl, targetBranch, job, options) {
  assert(process.env.CIRCLE_TOKEN, 'CIRCLE_TOKEN not found in environment')
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

async function buildAppVeyor (targetBranch, options) {
  console.log(`Triggering AppVeyor to run build on branch: ${targetBranch} with release flag.`)
  assert(process.env.APPVEYOR_TOKEN, 'APPVEYOR_TOKEN not found in environment')
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
      bearer: process.env.APPVEYOR_TOKEN
    },
    headers: {
      'Content-Type': 'application/json'
    },
    body: JSON.stringify({
      accountName: 'AppVeyor',
      projectSlug: 'electron',
      branch: targetBranch,
      environmentVariables
    }),
    method: 'POST'
  }
  let appVeyorResponse = await makeRequest(requestOpts, true).catch(err => {
    console.log('Error calling AppVeyor:', err)
  })
  const buildUrl = `https://windows-ci.electronjs.org/project/AppVeyor/electron/build/${appVeyorResponse.version}`
  console.log(`AppVeyor release build request successful.  Check build status at ${buildUrl}`)
}

function buildCircleCI (targetBranch, options) {
  const circleBuildUrl = `https://circleci.com/api/v1.1/project/github/electron/electron/tree/${targetBranch}?circle-token=${process.env.CIRCLE_TOKEN}`
  if (options.job) {
    assert(circleCIJobs.includes(options.job), `Unknown CI job name: ${options.job}.`)
    circleCIcall(circleBuildUrl, targetBranch, options.job, options)
  } else {
    circleCIJobs.forEach((job) => circleCIcall(circleBuildUrl, targetBranch, job, options))
  }
}

async function buildVSTS (targetBranch, options) {
  if (options.job) {
    assert(vstsJobs.includes(options.job), `Unknown CI job name: ${options.job}.`)
  }
  console.log(`Triggering VSTS to run build on branch: ${targetBranch} with release flag.`)
  assert(process.env.VSTS_TOKEN, 'VSTS_TOKEN not found in environment')
  let environmentVariables = {}

  if (!options.ghRelease) {
    environmentVariables.UPLOAD_TO_S3 = 1
  }

  if (options.automaticRelease) {
    environmentVariables.AUTO_RELEASE = 'true'
  }

  let requestOpts = {
    url: `${vstsURL}/definitions?api-version=4.1`,
    auth: {
      user: '',
      password: process.env.VSTS_TOKEN
    },
    headers: {
      'Content-Type': 'application/json'
    }
  }
  let vstsResponse = await makeRequest(requestOpts, true).catch(err => {
    console.log('Error calling VSTS to get build definitions:', err)
  })
  let buildsToRun = []
  if (options.job) {
    buildsToRun = vstsResponse.value.filter(build => build.name === options.job)
  } else {
    buildsToRun = vstsResponse.value.filter(build => vstsJobs.includes(build.name))
  }
  buildsToRun.forEach((build) => callVSTSBuild(build, targetBranch, environmentVariables))
}

async function callVSTSBuild (build, targetBranch, environmentVariables) {
  let buildBody = {
    definition: build,
    sourceBranch: targetBranch
  }
  if (Object.keys(environmentVariables).length !== 0) {
    buildBody.parameters = JSON.stringify(environmentVariables)
  }
  let requestOpts = {
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
  let vstsResponse = await makeRequest(requestOpts, true).catch(err => {
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
    buildVSTS(targetBranch, options)
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
    Usage: ci-release-build.js [--job=CI_JOB_NAME] [--ci=CircleCI|AppVeyor|VSTS] [--ghRelease] [--automaticRelease] TARGET_BRANCH
    `)
    process.exit(0)
  }
  runRelease(targetBranch, args)
}
