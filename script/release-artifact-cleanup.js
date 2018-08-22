#!/usr/bin/env node

if (!process.env.CI) require('dotenv-safe').load()
require('colors')
const args = require('minimist')(process.argv.slice(2), {
  boolean: ['tag']
})
const { exec, execSync } = require('child_process')
const fail = '\u2717'.red
const { GitProcess } = require('dugite')

const GitHub = require('github')
const pass = '\u2713'.green
const path = require('path')

const github = new GitHub()
const gitDir = path.resolve(__dirname, '..')

github.authenticate({
  type: 'token',
  token: process.env.ELECTRON_GITHUB_TOKEN
})

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

async function revertBumpCommit (tag) {
  let branch = getCurrentBranch()
  let hashToRevert = await exec(`git log | grep 'Bump v[0-9.]*' | grep -o '\\w\\ {8,\\}' | head -n 1`)
  await GitProcess.exec(['revert', hashToRevert], gitDir)
  let pushDetails = await GitProcess.exec(['push', 'origin', `HEAD:${branch}`, '--follow-tags'], gitDir)
  if (pushDetails.exitCode === 0) {
    console.log(`${pass} Successfully reverted release commit.`)
  } else {
    console.log(`${fail} Failed to push release commit: ` +
      `${pushDetails.stderr}`)
    process.exit(1)
  }
}

// async function deleteDraft (tag) {

// }

async function deleteTag (tag, targetRepo) {
  const result = await (github.gitdata.deleteReference({
    owner: 'electron',
    repo: targetRepo,
    ref: tag
  }))

  if (result.exitCode === 0) {
    console.log(`Successfully deleted tag ${tag} from 'electron/electron'`)
  } else {
    console.log(`Couldn't delete tag ${tag} from 'electron/electron'`)
  }
}

async function cleanReleaseArtifacts () {
  const tag = args.tag

  // delete tag from nightlies repo
  const lastBumpCommit = execSync(`git log | grep 'Bump v[0-9.]*' | head -n 1`).toString()

  if (lastBumpCommit.indexOf('nightly' > 0)) {
    await deleteTag(tag, 'nightlies')
  }

  // delete tag from `electron/electron`
  await deleteTag(tag, 'electron')

  // delete release
  // await deleteDraft(tag)

  // revert commit in `electron/electron`
  await revertBumpCommit()

  console.log('Failed release artifact cleanup complete')
}

cleanReleaseArtifacts()
