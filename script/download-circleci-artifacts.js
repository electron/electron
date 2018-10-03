const args = require('minimist')(process.argv.slice(2))
const nugget = require('nugget')
const request = require('request')

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

async function downloadArtifact (name, buildNum, dest) {
  const circleArtifactUrl = `https://circleci.com/api/v1.1/project/github/electron/electron/${args.buildNum}/artifacts?circle-token=${process.env.CIRCLE_TOKEN}`
  const artifacts = await makeRequest({
    method: 'GET',
    url: circleArtifactUrl,
    headers: {
      'Content-Type': 'application/json',
      'Accept': 'application/json'
    }
  }, true).catch(err => {
    console.log('Error calling CircleCI:', err)
  })
  const artifactToDownload = artifacts.find(artifact => {
    return (artifact.path === name)
  })
  if (!artifactToDownload) {
    console.log(`Could not find artifact called ${name} to download for build #${buildNum}.`)
    process.exit(1)
  } else {
    console.log(`Downloading ${artifactToDownload.url}.`)
    await downloadFile(artifactToDownload.url, dest)
    console.log(`Successfully downloaded ${name}.`)
  }
}

function downloadFile (url, directory) {
  return new Promise((resolve, reject) => {
    const nuggetOpts = {
      dir: directory
    }
    nugget(url, nuggetOpts, (err) => {
      if (err) {
        reject(err)
      } else {
        resolve()
      }
    })
  })
}

if (!args.name || !args.buildNum || !args.dest) {
  console.log(`Download CircleCI artifacts.
    Usage: download-circleci-artifacts.js [--buildNum=CIRCLE_BUILD_NUMBER] [--name=artifactName] [--dest]`)
  process.exit(0)
} else {
  downloadArtifact(args.name, args.buildNum, args.dest)
}
