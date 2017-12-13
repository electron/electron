#!/usr/bin/env node

require('colors')
const args = require('minimist')(process.argv.slice(2))
const assert = require('assert')
const ciReleaseBuild = require('./ci-release-build')
const { execSync } = require('child_process')
const fail = '\u2717'.red
const { GitProcess, GitError } = require('dugite')
const GitHub = require('github')
const pass = '\u2713'.green
const path = require('path')
const pkg = require('../package.json')
const versionType = args._[0]

// TODO (future) automatically determine version based on conventional commits
// via conventional-recommended-bump

assert(process.env.ELECTRON_GITHUB_TOKEN, 'ELECTRON_GITHUB_TOKEN not found in environment')
if (!versionType && !args.notesOnly) {
  console.log(`Usage: prepare-release versionType [major | minor | patch | beta]` +
     ` (--stable) (--notesOnly)`)
  process.exit(1)
}

const github = new GitHub()
const gitDir = path.resolve(__dirname, '..')
github.authenticate({type: 'token', token: process.env.ELECTRON_GITHUB_TOKEN})

async function createReleaseBranch () {
  console.log(`Creating release branch.`)
  let checkoutDetails = await GitProcess.exec([ 'checkout', '-b', 'release' ], gitDir)
  if (checkoutDetails.exitCode === 0) {
    console.log(`${pass} Successfully created the release branch.`)
  } else {
    const error = GitProcess.parseError(checkoutDetails.stderr)
    if (error === GitError.BranchAlreadyExists) {
      console.log(`${fail} Release branch already exists, aborting prepare ` +
        `release process.`)
    } else {
      console.log(`${fail} Error creating release branch: ` +
        `${checkoutDetails.stderr}`)
    }
    process.exit(1)
  }
}

function getNewVersion () {
  console.log(`Bumping for new "${versionType}" version.`)
  let bumpScript = path.join(__dirname, 'bump-version.py')
  let scriptArgs = [bumpScript, `--bump ${versionType}`]
  if (args.stable) {
    scriptArgs.push('--stable')
  }
  try {
    let bumpVersion = execSync(scriptArgs.join(' '), {encoding: 'UTF-8'})
    bumpVersion = bumpVersion.substr(bumpVersion.indexOf(':') + 1).trim()
    let newVersion = `v${bumpVersion}`
    console.log(`${pass} Successfully bumped version to ${newVersion}`)
    return newVersion
  } catch (err) {
    console.log(`${fail} Could not bump version, error was:`, err)
  }
}

async function getCurrentBranch (gitDir) {
  console.log(`Determining current git branch`)
  let gitArgs = ['rev-parse', '--abbrev-ref', 'HEAD']
  let branchDetails = await GitProcess.exec(gitArgs, gitDir)
  if (branchDetails.exitCode === 0) {
    let currentBranch = branchDetails.stdout.trim()
    console.log(`${pass} Successfully determined current git branch is ` +
      `${currentBranch}`)
    return currentBranch
  } else {
    let error = GitProcess.parseError(branchDetails.stderr)
    console.log(`${fail} Could not get details for the current branch,
      error was ${branchDetails.stderr}`, error)
    process.exit(1)
  }
}

async function getReleaseNotes (currentBranch) {
  console.log(`Generating release notes for ${currentBranch}.`)
  let githubOpts = {
    owner: 'electron',
    repo: 'electron',
    base: `v${pkg.version}`,
    head: currentBranch
  }
  let releaseNotes = '(placeholder)\n'
  console.log(`Checking for commits from ${pkg.version} to ${currentBranch}`)
  let commitComparison = await github.repos.compareCommits(githubOpts)
    .catch(err => {
      console.log(`{$fail} Error checking for commits from ${pkg.version} to ` +
        `${currentBranch}`, err)
      process.exit(1)
    })

  commitComparison.data.commits.forEach(commitEntry => {
    let commitMessage = commitEntry.commit.message
    if (commitMessage.toLowerCase().indexOf('merge') > -1) {
      releaseNotes += `${commitMessage} \n`
    }
  })
  console.log(`${pass} Done generating release notes for ${currentBranch}.`)
  return releaseNotes
}

async function createRelease (branchToTarget, isBeta) {
  let releaseNotes = await getReleaseNotes(branchToTarget)
  let newVersion = getNewVersion()
  const githubOpts = {
    owner: 'electron',
    repo: 'electron'
  }
  console.log(`Checking for existing draft release.`)
  let releases = await github.repos.getReleases(githubOpts)
    .catch(err => {
      console.log('$fail} Could not get releases.  Error was', err)
    })
  let drafts = releases.data.filter(release => release.draft)
  if (drafts.length > 0) {
    console.log(`${fail} Aborting because draft release for
      ${drafts[0].tag_name} already exists.`)
    process.exit(1)
  }
  console.log(`${pass} A draft release does not exist; creating one.`)
  githubOpts.body = releaseNotes
  githubOpts.draft = true
  githubOpts.name = `electron ${newVersion}`
  if (isBeta) {
    githubOpts.body = `Note: This is a beta release.  Please file new issues ` +
      `for any bugs you find in it.\n \n This release is published to npm ` +
      `under the beta tag and can be installed via npm install electron@beta, ` +
      `or npm i electron@${newVersion.substr(1)}.`
    githubOpts.name = `${githubOpts.name}`
    githubOpts.prerelease = true
  }
  githubOpts.tag_name = newVersion
  githubOpts.target_commitish = branchToTarget
  await github.repos.createRelease(githubOpts)
    .catch(err => {
      console.log(`${fail} Error creating new release: `, err)
      process.exit(1)
    })
  console.log(`${pass} Draft release for ${newVersion} has been created.`)
}

async function pushRelease () {
  let pushDetails = await GitProcess.exec(['push', 'origin', 'HEAD'], gitDir)
  if (pushDetails.exitCode === 0) {
    console.log(`${pass} Successfully pushed the release branch.  Wait for ` +
      `release builds to finish before running "npm run release".`)
  } else {
    console.log(`${fail} Error pushing the release branch: ` +
        `${pushDetails.stderr}`)
    process.exit(1)
  }
}

async function runReleaseBuilds () {
  await ciReleaseBuild('release', {
    ghRelease: true
  })
}

async function prepareRelease (isBeta, notesOnly) {
  let currentBranch = await getCurrentBranch(gitDir)
  if (notesOnly) {
    let releaseNotes = await getReleaseNotes(currentBranch)
    console.log(`Draft release notes are: ${releaseNotes}`)
  } else {
    await createReleaseBranch()
    await createRelease(currentBranch, isBeta)
    await pushRelease()
    await runReleaseBuilds()
  }
}

prepareRelease(!args.stable, args.notesOnly)
