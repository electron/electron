#!/usr/bin/env node

require('colors')
const assert = require('assert')
const branchToRelease = process.argv[2]
const fail = '\u2717'.red
const { GitProcess, GitError } = require('dugite')
const pass = '\u2713'.green
const path = require('path')
const pkg = require('../package.json')

assert(process.env.ELECTRON_GITHUB_TOKEN, 'ELECTRON_GITHUB_TOKEN not found in environment')
if (!branchToRelease) {
  console.log(`Usage: merge-release branch`)
  process.exit(1)
}
const gitDir = path.resolve(__dirname, '..')

async function callGit (args, errorMessage, successMessage) {
  let gitResult = await GitProcess.exec(args, gitDir)
  if (gitResult.exitCode === 0) {
    console.log(`${pass} ${successMessage}`)
    return true
  } else {
    console.log(`${fail} ${errorMessage} ${gitResult.stderr}`)
    process.exit(1)
  }
}

async function checkoutBranch (branchName) {
  console.log(`Checking out ${branchName}.`)
  let errorMessage = `Error checking out branch ${branchName}:`
  let successMessage = `Successfully checked out branch ${branchName}.`
  return callGit(['checkout', branchName], errorMessage, successMessage)
}

async function commitMerge () {
  console.log(`Committing the merge for v${pkg.version}`)
  let errorMessage = `Error committing merge:`
  let successMessage = `Successfully committed the merge for v${pkg.version}`
  let gitArgs = ['commit', '-m', `v${pkg.version}`]
  return callGit(gitArgs, errorMessage, successMessage)
}

async function mergeReleaseIntoBranch (branchName) {
  console.log(`Merging release branch into ${branchName}.`)
  let mergeArgs = ['merge', 'release', '--squash']
  let mergeDetails = await GitProcess.exec(mergeArgs, gitDir)
  if (mergeDetails.exitCode === 0) {
    return true
  } else {
    const error = GitProcess.parseError(mergeDetails.stderr)
    if (error === GitError.MergeConflicts) {
      console.log(`${fail} Could not merge release branch into ${branchName} ` +
        `due to merge conflicts.`)
      return false
    } else {
      console.log(`${fail} Could not merge release branch into ${branchName} ` +
        `due to an error: ${mergeDetails.stderr}.`)
      process.exit(1)
    }
  }
}

async function pushBranch (branchName) {
  console.log(`Pushing branch ${branchName}.`)
  let pushArgs = ['push', 'origin', branchName]
  let errorMessage = `Could not push branch ${branchName} due to an error:`
  let successMessage = `Successfully pushed branch ${branchName}.`
  return callGit(pushArgs, errorMessage, successMessage)
}

async function pull () {
  console.log(`Performing a git pull`)
  let errorMessage = `Could not pull due to an error:`
  let successMessage = `Successfully performed a git pull`
  return callGit(['pull'], errorMessage, successMessage)
}

async function rebase (targetBranch) {
  console.log(`Rebasing release branch from ${targetBranch}`)
  let errorMessage = `Could not rebase due to an error:`
  let successMessage = `Successfully rebased release branch from ` +
    `${targetBranch}`
  return callGit(['rebase', targetBranch], errorMessage, successMessage)
}

async function mergeRelease () {
  await checkoutBranch(branchToRelease)
  let mergeSuccess = await mergeReleaseIntoBranch(branchToRelease)
  if (mergeSuccess) {
    console.log(`${pass} Successfully merged release branch into ` +
      `${branchToRelease}.`)
    await commitMerge()
    let pushSuccess = await pushBranch(branchToRelease)
    if (pushSuccess) {
      console.log(`${pass} Success!!! ${branchToRelease} now has the latest release!`)
    }
  } else {
    console.log(`Trying rebase of ${branchToRelease} into release branch.`)
    await pull()
    await checkoutBranch('release')
    let rebaseResult = await rebase(branchToRelease)
    if (rebaseResult) {
      let pushResult = pushBranch('HEAD')
      if (pushResult) {
        console.log(`Rebase of ${branchToRelease} into release branch was ` +
          `successful.  Let release builds run and then try this step again.`)
      }
      // Exit as failure so release doesn't continue
      process.exit(1)
    }
  }
}

mergeRelease()
