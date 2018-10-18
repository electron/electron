#!/usr/bin/env node

if (!process.env.CI) require('dotenv-safe').load()
require('colors')
const args = require('minimist')(process.argv.slice(2), {
  string: ['tag']
})
const { execSync } = require('child_process')
const { GitProcess } = require('dugite')

const GitHub = require('github')
const path = require('path')

const github = new GitHub()
const gitDir = path.resolve(__dirname, '..')

github.authenticate({
  type: 'token',
  token: process.env.ELECTRON_GITHUB_TOKEN
})

function getLastBumpCommit (tag) {
  const data = execSync(`git log -n1 --grep "Bump ${tag}" --format='format:{"hash": "%H", "message": "%s"}'`).toString()
  return JSON.parse(data)
}

async function getCurrentBranch (gitDir) {
  const gitArgs = ['rev-parse', '--abbrev-ref', 'HEAD']
  const branchDetails = await GitProcess.exec(gitArgs, gitDir)
  if (branchDetails.exitCode === 0) {
    return branchDetails.stdout.trim()
  }

  const error = GitProcess.parseError(branchDetails.stderr)
  console.error(`Couldn't get current branch: `, error)
  process.exit(1)
}

async function revertBumpCommit (tag) {
  const branch = await getCurrentBranch()
  const commitToRevert = getLastBumpCommit(tag).hash
  await GitProcess.exec(['revert', commitToRevert], gitDir)
  const pushDetails = await GitProcess.exec(['push', 'origin', `HEAD:${branch}`, '--follow-tags'], gitDir)
  if (pushDetails.exitCode === 0) {
    console.log(`Successfully reverted release commit.`)
  } else {
    const error = GitProcess.parseError(pushDetails.stderr)
    console.error(`Failed to push release commit: `, error)
    process.exit(1)
  }
}

async function deleteDraft (tag, targetRepo) {
  try {
    const result = await github.repos.getReleaseByTag({
      owner: 'electron',
      repo: targetRepo,
      tag
    })
    if (!result.draft) {
      console.log(`Published releases cannot be deleted.`)
      process.exit(1)
    } else {
      await github.repos.deleteRelease({
        owner: 'electron',
        repo: targetRepo,
        release_id: result.id
      })
    }
    console.log(`Successfully deleted draft with tag ${tag} from ${targetRepo}`)
  } catch (err) {
    console.error(`Couldn't delete draft with tag ${tag} from ${targetRepo}: `, err)
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
    console.log(`Successfully deleted tag ${tag} from ${targetRepo}`)
  } catch (err) {
    console.log(`Couldn't delete tag ${tag} from ${targetRepo}: `, err)
    process.exit(1)
  }
}

async function cleanReleaseArtifacts () {
  const tag = args.tag
  const isNightly = args.tag.includes('nightly')

  if (isNightly) {
    await deleteDraft(tag, 'nightlies')
    await deleteTag(tag, 'nightlies')
  } else {
    await deleteDraft(tag, 'electron')
  }

  await deleteTag(tag, 'electron')
  await revertBumpCommit(tag)

  console.log('Failed release artifact cleanup complete')
}

cleanReleaseArtifacts()
