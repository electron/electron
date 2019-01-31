if (!process.env.CI) require('dotenv-safe').load()

const fs = require('fs')

const octokit = require('@octokit/rest')()
octokit.authenticate({ type: 'token', token: process.env.ELECTRON_GITHUB_TOKEN })

if (process.argv.length < 6) {
  console.log('Usage: upload-to-github filePath fileName releaseId')
  process.exit(1)
}

const filePath = process.argv[2]
const fileName = process.argv[3]
const releaseId = process.argv[4]
const releaseVersion = process.argv[5]

const getHeaders = (filePath, fileName) => {
  const extension = fileName.split('.').pop()
  const size = fs.statSync(filePath).size
  const options = {
    'json': 'text/json',
    'zip': 'application/zip',
    'txt': 'text/plain',
    'ts': 'application/typescript'
  }

  return {
    'content-type': options[extension],
    'content-length': size
  }
}

const targetRepo = releaseVersion.indexOf('nightly') > 0 ? 'nightlies' : 'electron'
const uploadUrl = `https://uploads.github.com/repos/electron/${targetRepo}/releases/${releaseId}/assets{?name,label}`
let retry = 0

function uploadToGitHub () {
  const fakeFileNamePrefix = `fake-${fileName}-fake-`
  const fakeFileName = `${fakeFileNamePrefix}${Date.now()}`

  octokit.repos.uploadReleaseAsset({
    url: uploadUrl,
    headers: getHeaders(filePath, fileName),
    file: fs.createReadStream(filePath),
    name: fakeFileName
  }).then((uploadResponse) => {
    console.log(`Successfully uploaded ${fileName} to GitHub as ${fakeFileName}. Going for the rename now.`)
    return octokit.repos.updateReleaseAsset({
      owner: 'electron',
      repo: 'electron',
      asset_id: uploadResponse.data.id,
      name: fileName
    }).then(() => {
      console.log(`Successfully renamed ${fakeFileName} to ${fileName}. All done now.`)
      process.exit(0)
    })
  }).catch((err) => {
    if (retry < 4) {
      console.log(`Error uploading ${fileName} as ${fakeFileName} to GitHub, will retry.  Error was:`, err)
      retry++

      octokit.repos.listAssetsForRelease({
        owner: 'electron',
        repo: targetRepo,
        release_id: releaseId
      }).then(assets => {
        console.log('Got list of assets for existing release:')
        console.log(JSON.stringify(assets.data, null, '  '))
        const existingAssets = assets.data.filter(asset => asset.name.startsWith(fakeFileNamePrefix) || asset.name === fileName)

        if (existingAssets.length > 0) {
          console.log(`${fileName} already exists; will delete before retrying upload.`)
          Promise.all(
            existingAssets.map(existingAsset => octokit.repos.deleteReleaseAsset({
              owner: 'electron',
              repo: targetRepo,
              asset_id: existingAsset.id
            }))
          ).catch((deleteErr) => {
            console.log(`Failed to delete existing asset ${fileName}.  Error was:`, deleteErr)
          }).then(uploadToGitHub)
        } else {
          console.log(`Current asset ${fileName} not found in existing assets; retrying upload.`)
          uploadToGitHub()
        }
      }).catch((getReleaseErr) => {
        console.log(`Fatal: Unable to get current release assets via getRelease!  Error was:`, getReleaseErr)
      })
    } else {
      console.log(`Error retrying uploading ${fileName} to GitHub:`, err)
      process.exitCode = 1
    }
  })
}

uploadToGitHub()
