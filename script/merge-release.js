#!/usr/bin/env node
const assert = require('assert')
const branchToRelease = process.argv[2]
const { GitProcess, GitError } = require('dugite')
const GitHub = require('github')
const path = require('path')

assert(process.env.ELECTRON_GITHUB_TOKEN, 'ELECTRON_GITHUB_TOKEN not found in environment')
if (!branchToRelease) {
  console.log(`Usage: finish-release branch`)
  process.exit(1)
}

const github = new GitHub()
const gitDir = path.resolve(__dirname, '..')
github.authenticate({type: 'token', token: process.env.ELECTRON_GITHUB_TOKEN})

async function callGit (args, errorMessage) {
  let gitResult = await GitProcess.exec(args, gitDir)
  if (gitResult.exitCode === 0) {
    return true
  } else {
    console.log(`${errorMessage} ${gitResult.stderr}`)
    process.exit(1)
  }
}

async function checkoutBranch (branchName) {
  let errorMessage = `Error checking out branch ${branchName}:`
  return await callGit(['checkout', branchName], errorMessage)
}

async function mergeReleaseIntoBranch (branchName) {
  let mergeArgs = ['merge', 'release', '--no-commit']
  let mergeDetails = await GitProcess.exec(mergeArgs, gitDir)
  if (mergeDetails.exitCode === 0) {
    return true
  } else {
    const error = GitProcess.parseError(mergeDetails.stderr)
    if (error === GitError.MergeConflicts) {
      console.log(`Could not merge release branch into ${branchName} due to ` +
        `merge conflicts`)
      return false
    } else {
      console.log(`Could not merge release branch into ${branchName} due to ` +
        `an error: ${mergeDetails.stderr}`)
      process.exit(1)
    }
  }
}

async function pushBranch (branchName) {
  let pushArgs = ['push', 'origin', branchName]
  let errorMessage = `Could not push branch ${branchName} due to an error:`
  return await callGit(pushArgs, errorMessage)
}

async function pull () {
  let errorMessage = `Could not pull due to an error:`
  return await callGit(['pull'], errorMessage)
}

async function rebase (targetBranch) {
  let errorMessage = `Could not rebase due to an error:`
  return await callGit(['rebase', targetBranch], errorMessage)
}

async function mergeRelease () {
  await checkoutBranch(branchToRelease)
  let mergeSuccess = await mergeReleaseIntoBranch(branchToRelease)
  if (mergeSuccess) {
    console.log(`Successfully merged release branch into ${branchToRelease}.`)
    let pushSuccess = await pushBranch(branchToRelease)
    if (pushSuccess) {
      console.log(`Success!!! ${branchToRelease} now has the latest release!`)
    }
  } else {
    console.log(`Trying rebase of ${branchToRelease} into release branch.`)
    await pull()
    await checkoutBranch('release')
    let rebaseResult = await rebase(branchToRelease)
    if (rebaseResult) {
      console.log(`Rebase of ${branchToRelease} into release branch was ` +
        `successful.`)
      pushBranch('HEAD')
    }
  }
}

mergeRelease()
