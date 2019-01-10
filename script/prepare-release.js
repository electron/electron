#!/usr/bin/env node

if (!process.env.CI) require('dotenv-safe').load()
const args = require('minimist')(process.argv.slice(2), {
  boolean: ['automaticRelease', 'notesOnly', 'stable']
})
const ciReleaseBuild = require('./ci-release-build')
const octokit = require('@octokit/rest')()
const { execSync } = require('child_process')
const { GitProcess } = require('dugite')

const path = require('path')
const readline = require('readline')
const releaseNotesGenerator = require('./release-notes/index.js')
const { getCurrentBranch } = require('./lib/utils.js')
const bumpType = args._[0]
const targetRepo = bumpType === 'nightly' ? 'nightlies' : 'electron'

require('colors')
const pass = '\u2713'.green
const fail = '\u2717'.red

if (!bumpType && !args.notesOnly) {
  console.log(`Usage: prepare-release [stable | beta | nightly]` +
     ` (--stable) (--notesOnly) (--automaticRelease) (--branch)`)
  process.exit(1)
}

const gitDir = path.resolve(__dirname, '..')
octokit.authenticate({ type: 'token', token: process.env.ELECTRON_GITHUB_TOKEN })

async function getNewVersion (dryRun) {
  if (!dryRun) {
    console.log(`Bumping for new "${bumpType}" version.`)
  }
  const bumpScript = path.join(__dirname, 'bump-version.js')
  const scriptArgs = ['node', bumpScript, `--bump=${bumpType}`]
  if (dryRun) scriptArgs.push('--dryRun')
  try {
    let bumpVersion = execSync(scriptArgs.join(' '), { encoding: 'UTF-8' })
    bumpVersion = bumpVersion.substr(bumpVersion.indexOf(':') + 1).trim()
    const newVersion = `v${bumpVersion}`
    if (!dryRun) {
      console.log(`${pass} Successfully bumped version to ${newVersion}`)
    }
    return newVersion
  } catch (err) {
    console.log(`${fail} Could not bump version, error was:`, err)
    throw err
  }
}

async function getReleaseNotes (currentBranch) {
  if (bumpType === 'nightly') {
    return { text: 'Nightlies do not get release notes, please compare tags for info.' }
  }
  console.log(`Generating release notes for ${currentBranch}.`)
  const releaseNotes = await releaseNotesGenerator(currentBranch)
  if (releaseNotes.warning) {
    console.warn(releaseNotes.warning)
  }
  return releaseNotes
}

async function createRelease (branchToTarget, isBeta) {
  const releaseNotes = await getReleaseNotes(branchToTarget)
  const newVersion = await getNewVersion()
  await tagRelease(newVersion)

  console.log(`Checking for existing draft release.`)
  const releases = await octokit.repos.listReleases({
    owner: 'electron',
    repo: targetRepo
  }).catch(err => {
    console.log(`${fail} Could not get releases. Error was: `, err)
  })

  const drafts = releases.data.filter(release => release.draft &&
    release.tag_name === newVersion)
  if (drafts.length > 0) {
    console.log(`${fail} Aborting because draft release for
      ${drafts[0].tag_name} already exists.`)
    process.exit(1)
  }
  console.log(`${pass} A draft release does not exist; creating one.`)

  let releaseBody
  let releaseIsPrelease = false
  if (isBeta) {
    if (newVersion.indexOf('nightly') > 0) {
      releaseBody = `Note: This is a nightly release.  Please file new issues ` +
        `for any bugs you find in it.\n \n This release is published to npm ` +
        `under the nightly tag and can be installed via npm install electron@nightly, ` +
        `or npm i electron@${newVersion.substr(1)}.\n \n ${releaseNotes.text}`
    } else {
      releaseBody = `Note: This is a beta release.  Please file new issues ` +
        `for any bugs you find in it.\n \n This release is published to npm ` +
        `under the beta tag and can be installed via npm install electron@beta, ` +
        `or npm i electron@${newVersion.substr(1)}.\n \n ${releaseNotes.text}`
    }
    releaseIsPrelease = true
  } else {
    releaseBody = releaseNotes
  }

  const release = await octokit.repos.createRelease({
    owner: 'electron',
    repo: targetRepo,
    tag_name: newVersion,
    draft: true,
    name: `electron ${newVersion}`,
    body: releaseBody,
    prerelease: releaseIsPrelease,
    target_commitish: newVersion.indexOf('nightly') !== -1 ? 'master' : branchToTarget
  }).catch(err => {
    console.log(`${fail} Error creating new release: `, err)
    process.exit(1)
  })

  console.log(`Release has been created with id: ${release.data.id}.`)
  console.log(`${pass} Draft release for ${newVersion} successful.`)
}

async function pushRelease (branch) {
  const pushDetails = await GitProcess.exec(['push', 'origin', `HEAD:${branch}`, '--follow-tags'], gitDir)
  if (pushDetails.exitCode === 0) {
    console.log(`${pass} Successfully pushed the release.  Wait for ` +
      `release builds to finish before running "npm run release".`)
  } else {
    console.log(`${fail} Error pushing the release: ${pushDetails.stderr}`)
    process.exit(1)
  }
}

async function runReleaseBuilds (branch) {
  await ciReleaseBuild(branch, {
    ghRelease: true,
    automaticRelease: args.automaticRelease
  })
}

async function tagRelease (version) {
  console.log(`Tagging release ${version}.`)
  const checkoutDetails = await GitProcess.exec([ 'tag', '-a', '-m', version, version ], gitDir)
  if (checkoutDetails.exitCode === 0) {
    console.log(`${pass} Successfully tagged ${version}.`)
  } else {
    console.log(`${fail} Error tagging ${version}: ` +
      `${checkoutDetails.stderr}`)
    process.exit(1)
  }
}

async function verifyNewVersion () {
  const newVersion = await getNewVersion(true)
  let response
  if (args.automaticRelease) {
    response = 'y'
  } else {
    response = await promptForVersion(newVersion)
  }
  if (response.match(/^y/i)) {
    console.log(`${pass} Starting release of ${newVersion}`)
  } else {
    console.log(`${fail} Aborting release of ${newVersion}`)
    process.exit()
  }
}

async function promptForVersion (version) {
  return new Promise((resolve, reject) => {
    const rl = readline.createInterface({
      input: process.stdin,
      output: process.stdout
    })
    rl.question(`Do you want to create the release ${version.green} (y/N)? `, (answer) => {
      rl.close()
      resolve(answer)
    })
  })
}

// function to determine if there have been commits to master since the last release
async function changesToRelease () {
  const lastCommitWasRelease = new RegExp(`^Bump v[0-9.]*(-beta[0-9.]*)?(-nightly[0-9.]*)?$`, 'g')
  const lastCommit = await GitProcess.exec(['log', '-n', '1', `--pretty=format:'%s'`], gitDir)
  return !lastCommitWasRelease.test(lastCommit.stdout)
}

async function prepareRelease (isBeta, notesOnly) {
  if (args.dryRun) {
    const newVersion = await getNewVersion(true)
    console.log(newVersion)
  } else {
    const currentBranch = (args.branch) ? args.branch : await getCurrentBranch(gitDir)
    if (notesOnly) {
      const releaseNotes = await getReleaseNotes(currentBranch)
      console.log(`Draft release notes are: \n${releaseNotes.text}`)
    } else {
      const changes = await changesToRelease(currentBranch)
      if (changes) {
        await verifyNewVersion()
        await createRelease(currentBranch, isBeta)
        await pushRelease(currentBranch)
        await runReleaseBuilds(currentBranch)
      } else {
        console.log(`There are no new changes to this branch since the last release, aborting release.`)
        process.exit(1)
      }
    }
  }
}

prepareRelease(!args.stable, args.notesOnly)
