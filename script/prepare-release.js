#!/usr/bin/env node
const args = require('minimist')(process.argv.slice(2))
const assert = require('assert')
const { execSync } = require('child_process')
const { GitProcess, GitError } = require('dugite')
const GitHub = require('github')
const path = require('path')
const pkg = require('../package.json')
const versionType = args._[0]

// TODO (future) automatically determine version based on conventional commits
// via conventional-recommended-bump

assert(process.env.ELECTRON_GITHUB_TOKEN, 'ELECTRON_GITHUB_TOKEN not found in environment')
if (!versionType) {
  console.log(`Usage: prepare-release versionType [major | minor | patch | beta]` +
     ` (--stable) (--notesOnly)`)
  process.exit(1)
}

const github = new GitHub()
const gitDir = path.resolve(__dirname, '..')
github.authenticate({type: 'token', token: process.env.ELECTRON_GITHUB_TOKEN})

async function createReleaseBranch () {
  let checkoutDetails = await GitProcess.exec([ 'checkout', '-b', 'release' ], gitDir)
  if (checkoutDetails.exitCode === 0) {
    console.log(`Release branch success: ${checkoutDetails.stdout}`)
  } else {
    const error = GitProcess.parseError(checkoutDetails.stderr)
    if (error === GitError.BranchAlreadyExists) {
      console.log(`release branch already exists, aborting release process.`)
      process.exit(1)
    } else {
      console.log(`Error creating release branch: ${checkoutDetails.stderr}`)
    }
  }
}

function getNewVersion () {
  let bumpScript = path.join(__dirname, 'bump-version.py')
  let scriptArgs = [bumpScript, `--bump ${versionType}`]
  if (args.stable) {
    scriptArgs.push('--stable')
  }
  let newVersion = execSync(scriptArgs.join(' '), {encoding: 'UTF-8'})
  newVersion = newVersion.substr(newVersion.indexOf(':') + 1).trim()
  return `v${newVersion}`
}

async function getCurrentBranch (gitDir) {
  let gitArgs = ['rev-parse', '--abbrev-ref', 'HEAD']
  let branchDetails = await GitProcess.exec(gitArgs, gitDir)
  if (branchDetails.exitCode === 0) {
    return branchDetails.stdout.trim()
  } else {
    let error = GitProcess.parseError(branchDetails.stderr)
    console.log(`Could not get details for the current branch,
      error was ${branchDetails.stderr}`, error)
    process.exit(1)
  }
}

async function getReleaseNotes (currentBranch) {
  let githubOpts = {
    owner: 'electron',
    repo: 'electron',
    base: `v${pkg.version}`,
    head: currentBranch
  }
  let releaseNotes = ''
  console.log(`Checking for commits from ${pkg.version} to ${currentBranch}`)
  let commitComparison = await github.repos.compareCommits(githubOpts)
    .catch(err => {
      console.log(`Error checking for commits from ${pkg.version} to ${currentBranch}`, err)
      process.exit(1)
    })

  commitComparison.data.commits.forEach(commitEntry => {
    let commitMessage = commitEntry.commit.message
    if (commitMessage.toLowerCase().indexOf('merge') > -1) {
      releaseNotes += `${commitMessage} \n`
    }
  })
  return releaseNotes
}

async function createRelease (branchToTarget, isBeta) {
  let releaseNotes = await getReleaseNotes(branchToTarget)
  let newVersion = getNewVersion()
  console.log(`Gathered data for new version ${newVersion}, release notes are: \n${releaseNotes}`)

  const githubOpts = {
    owner: 'electron',
    repo: 'electron'
  }
  let releases = await github.repos.getReleases(githubOpts)
    .catch(err => {
      console.log('Could not get releases.  Error was', err)
    })
  let drafts = releases.data.filter(release => release.draft)
  if (drafts.length > 0) {
    console.log(`Aborting because draft release for
      ${drafts[0].release.tag_name} already exists.`)
    process.exit(1)
  }
  githubOpts.body = releaseNotes
  githubOpts.draft = true
  githubOpts.name = `electron ${newVersion}`
  if (isBeta) {
    githubOpts.name = `${githubOpts.name} beta`
    githubOpts.prerelease = true
  }
  githubOpts.tag_name = newVersion
  githubOpts.target_commitish = branchToTarget
  let releaseCreated = await github.repos.createRelease(githubOpts)
    .catch(err => {
      console.log('Error creating new release: ', err)
      process.exit(1)
    })
  console.log(`Draft release for ${newVersion} has been created.  Push this
    branch to kick off release builds.`, releaseCreated)
}

async function prepareRelease (isBeta, notesOnly) {
  let currentBranch = await getCurrentBranch(gitDir)
  if (notesOnly) {
    let releaseNotes = await getReleaseNotes(currentBranch)
    console.log(`Draft release notes are: ${releaseNotes}`)
  } else {
    await createReleaseBranch()
    await createRelease(currentBranch, !args.stable)
  }
}

prepareRelease(!args.stable, args.notesOnly)
