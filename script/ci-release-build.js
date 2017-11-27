const assert = require('assert')
const request = require('request')
const buildAppVeyorURL = 'https://windows-ci.electronjs.org/api/builds'
const jenkinsServer = 'https://mac-ci.electronjs.org'

const circleCIJobs = [
  'electron-linux-arm',
  'electron-linux-arm64',
  'electron-linux-ia32',
  'electron-linux-mips64el',
  'electron-linux-x64'
]

const jenkinsJobs = [
  'electron-mas-x64-release',
  'electron-osx-x64-release'
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

async function circleCIcall (buildUrl, targetBranch, job, ghRelease) {
  assert(process.env.CIRCLE_TOKEN, 'CIRCLE_TOKEN not found in environment')
  console.log(`Triggering CircleCI to run build job: ${job} on branch: ${targetBranch} with release flag.`)
  let buildRequest = {
    'build_parameters': {
      'CIRCLE_JOB': job
    }
  }

  if (ghRelease) {
    buildRequest.build_parameters.ELECTRON_RELEASE = 1
  } else {
    buildRequest.build_parameters.RUN_RELEASE_BUILD = 'true'
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
  console.log(`Check ${circleResponse.build_url} for status. (${job})`)
}

async function buildAppVeyor (targetBranch, ghRelease) {
  console.log(`Triggering AppVeyor to run build on branch: ${targetBranch} with release flag.`)
  assert(process.env.APPVEYOR_TOKEN, 'APPVEYOR_TOKEN not found in environment')
  let environmentVariables = {}

  if (ghRelease) {
    environmentVariables.ELECTRON_RELEASE = 1
  } else {
    environmentVariables.RUN_RELEASE_BUILD = 'true'
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

function buildCircleCI (targetBranch, ghRelease, job) {
  const circleBuildUrl = `https://circleci.com/api/v1.1/project/github/electron/electron/tree/${targetBranch}?circle-token=${process.env.CIRCLE_TOKEN}`
  if (job) {
    assert(circleCIJobs.includes(job), `Unknown CI job name: ${job}.`)
    circleCIcall(circleBuildUrl, targetBranch, job, ghRelease)
  } else {
    circleCIJobs.forEach((job) => circleCIcall(circleBuildUrl, targetBranch, job, ghRelease))
  }
}

async function buildJenkins (targetBranch, ghRelease, job) {
  assert(process.env.JENKINS_AUTH_TOKEN, 'JENKINS_AUTH_TOKEN not found in environment')
  assert(process.env.JENKINS_BUILD_TOKEN, 'JENKINS_BUILD_TOKEN not found in environment')
  let jenkinsCrumb = await getJenkinsCrumb()

  if (job) {
    assert(jenkinsJobs.includes(job), `Unknown CI job name: ${job}.`)
    callJenkinsBuild(job, jenkinsCrumb, targetBranch, ghRelease)
  } else {
    jenkinsJobs.forEach((job) => {
      callJenkinsBuild(job, jenkinsCrumb, targetBranch, ghRelease)
    })
  }
}

async function callJenkins (path, requestParameters, requestHeaders) {
  let requestOptions = {
    url: `${jenkinsServer}/${path}`,
    auth: {
      user: 'build',
      pass: process.env.JENKINS_AUTH_TOKEN
    },
    qs: requestParameters
  }
  if (requestHeaders) {
    requestOptions.headers = requestHeaders
  }
  let jenkinsResponse = await makeRequest(requestOptions).catch(err => {
    console.log(`Error calling Jenkins:`, err)
  })
  return jenkinsResponse
}

async function callJenkinsBuild (job, jenkinsCrumb, targetBranch, ghRelease) {
  console.log(`Triggering Jenkins to run build job: ${job} on branch: ${targetBranch} with release flag.`)
  let jenkinsParams = {
    token: process.env.JENKINS_BUILD_TOKEN,
    BRANCH: targetBranch
  }
  if (!ghRelease) {
    jenkinsParams.RUN_RELEASE_BUILD = 1
  }
  await callJenkins(`job/${job}/buildWithParameters`, jenkinsParams, jenkinsCrumb)
    .catch(err => {
      console.log(`Error calling Jenkins build`, err)
    })
  let buildUrl = `${jenkinsServer}/job/${job}/lastBuild/`
  console.log(`Jenkins build request successful.  Check build status at ${buildUrl}.`)
}

async function getJenkinsCrumb () {
  let crumbResponse = await callJenkins('crumbIssuer/api/xml', {
    xpath: 'concat(//crumbRequestField,":",//crumb)'
  }).catch(err => {
    console.log(`Error getting jenkins crumb:`, err)
  })
  let crumbDetails = crumbResponse.split(':')
  let crumbHeader = {}
  crumbHeader[crumbDetails[0]] = crumbDetails[1]
  return crumbHeader
}

function runRelease (targetBranch, options) {
  if (options.ci) {
    switch (options.ci) {
      case 'CircleCI': {
        buildCircleCI(targetBranch, options.ghRelease, options.job)
        break
      }
      case 'AppVeyor': {
        buildAppVeyor(targetBranch, options.ghRelease)
        break
      }
      case 'Jenkins': {
        buildJenkins(targetBranch, options.ghRelease, options.job)
        break
      }
    }
  } else {
    buildCircleCI(targetBranch, options.ghRelease, options.job)
    buildAppVeyor(targetBranch, options.ghRelease)
    buildJenkins(targetBranch, options.ghRelease, options.job)
  }
}

module.exports = runRelease

if (require.main === module) {
  const args = require('minimist')(process.argv.slice(2))
  const targetBranch = args._[0]
  if (args._.length < 1) {
    console.log(`Trigger CI to build release builds of electron.
    Usage: ci-release-build.js [--job=CI_JOB_NAME] [--ci=CircleCI|AppVeyor|Jenkins] [--ghRelease] TARGET_BRANCH
    `)
    process.exit(0)
  }
  runRelease(targetBranch, args)
}
