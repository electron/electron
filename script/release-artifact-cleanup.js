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
  const gitArgs = ['rev-parse', '--abbrev-ref', 'HEAD']
  const branchDetails = await GitProcess.exec(gitArgs, gitDir)
  if (branchDetails.exitCode === 0) {
    return branchDetails.stdout.trim()
  } else {
    const error = GitProcess.parseError(branchDetails.stderr)
    console.log(`${fail} Couldn't get current branch: ${branchDetails.stderr}`, error)
    process.exit(1)
  }
}

async function revertBumpCommit (tag) {
  const branch = getCurrentBranch()
  const hashToRevert = await exec(`git log | grep 'Bump v[0-9.]*' | grep -o '\\w\\ {8,\\}' | head -n 1`)
  await GitProcess.exec(['revert', hashToRevert], gitDir)
  const pushDetails = await GitProcess.exec(['push', 'origin', `HEAD:${branch}`, '--follow-tags'], gitDir)
  if (pushDetails.exitCode === 0) {
    console.log(`${pass} Successfully reverted release commit.`)
  } else {
    console.log(`${fail} Failed to push release commit: ${pushDetails.stderr}`)
    process.exit(1)
  }
}

async function deleteDraft (tag, repo) {
  try {
    const result = await github.repos.getReleaseByTag({
      owner: 'electron',
      repo,
      tag
    })
    if (result.draft) {
      console.log(`Published drafts cannot be deleted.`)
      process.exit(1)
    } else {
      await github.repos.deleteRelease({
        owner: 'electron',
        repo,
        release_id: result.id
      })
    }
    console.log(`Successfully deleted draft with tag ${tag}`)
  } catch (err) {
    console.log(`Couldn't delete draft: ${err}`)
    process.exit(1)
  }
}

async function deleteTag (tag, targetRepo) {
  try {
    await github.gitdata.deleteReference({
      owner: 'electron',
      repo: targetRepo,
      ref: tag
    })
    console.log(`Successfully deleted tag ${tag} from 'electron/electron'`)
  } catch (err) {
    console.log(`Couldn't delete tag ${tag} from 'electron/electron'`)
    process.exit(1)
  }
}

async function cleanReleaseArtifacts () {
  const tag = args.tag
  const lastBumpCommit = execSync(`git log | grep 'Bump v[0-9.]*' | head -n 1`).toString()

  if (lastBumpCommit.indexOf('nightly' > 0)) {
    await deleteDraft(tag, 'nightlies')
    await deleteTag(tag, 'nightlies')
  } else {
    await deleteDraft(tag, 'electron')
  }

  await deleteTag(tag, 'electron')
  await revertBumpCommit()

  console.log('Failed release artifact cleanup complete')
}

cleanReleaseArtifacts()
