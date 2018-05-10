#!/usr/bin/env node

require('colors')
const args = require('minimist')(process.argv.slice(2), {
  boolean: ['automaticRelease', 'notesOnly', 'stable']
})
const assert = require('assert')
const ciReleaseBuild = require('./ci-release-build')
const { execSync } = require('child_process')
const fail = '\u2717'.red
const { GitProcess } = require('dugite')
const GitHub = require('github')
const pass = '\u2713'.green
const path = require('path')
const pkg = require('../package.json')
const readline = require('readline')
const versionType = args._[0]

// TODO (future) automatically determine version based on conventional commits
// via conventional-recommended-bump

assert(process.env.ELECTRON_GITHUB_TOKEN, 'ELECTRON_GITHUB_TOKEN not found in environment')
if (!versionType && !args.notesOnly) {
  console.log(`Usage: prepare-release versionType [major | minor | patch | beta]` +
     ` (--stable) (--notesOnly) (--automaticRelease) (--branch)`)
  process.exit(1)
}

const github = new GitHub()
const gitDir = path.resolve(__dirname, '..')
github.authenticate({type: 'token', token: process.env.ELECTRON_GITHUB_TOKEN})

function getNewVersion (dryRun) {
  console.log(`Bumping for new "${versionType}" version.`)
  let bumpScript = path.join(__dirname, 'bump-version.py')
  let scriptArgs = [bumpScript, `--bump ${versionType}`]
  if (args.stable) {
    scriptArgs.push('--stable')
  }
  if (dryRun) {
    scriptArgs.push('--dry-run')
  }
  try {
    let bumpVersion = execSync(scriptArgs.join(' '), {encoding: 'UTF-8'})
    bumpVersion = bumpVersion.substr(bumpVersion.indexOf(':') + 1).trim()
    let newVersion = `v${bumpVersion}`
    if (!dryRun) {
      console.log(`${pass} Successfully bumped version to ${newVersion}`)
    }
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
  let releaseNotes
  if (args.automaticRelease) {
    releaseNotes = '## Bug Fixes/Changes \n\n'
  } else {
    releaseNotes = '(placeholder)\n'
  }
  console.log(`Checking for commits from ${pkg.version} to ${currentBranch}`)
  let commitComparison = await github.repos.compareCommits(githubOpts)
    .catch(err => {
      console.log(`${fail} Error checking for commits from ${pkg.version} to ` +
        `${currentBranch}`, err)
      process.exit(1)
    })

  if (commitComparison.data.commits.length === 0) {
    console.log(`${pass} There are no commits from ${pkg.version} to ` +
      `${currentBranch}, skipping release.`)
    process.exit(0)
  }

  let prCount = 0
  const mergeRE = /Merge pull request #(\d+) from .*\n/
  const newlineRE = /(.*)\n*.*/
  const prRE = /(.* )\(#(\d+)\)(?:.*)/
  commitComparison.data.commits.forEach(commitEntry => {
    let commitMessage = commitEntry.commit.message
    if (commitMessage.indexOf('#') > -1) {
      let prMatch = commitMessage.match(mergeRE)
      let prNumber
      if (prMatch) {
        commitMessage = commitMessage.replace(mergeRE, '').replace('\n', '')
        let newlineMatch = commitMessage.match(newlineRE)
        if (newlineMatch) {
          commitMessage = newlineMatch[1]
        }
        prNumber = prMatch[1]
      } else {
        prMatch = commitMessage.match(prRE)
        if (prMatch) {
          commitMessage = prMatch[1].trim()
          prNumber = prMatch[2]
        }
      }
      if (prMatch) {
        if (commitMessage.substr(commitMessage.length - 1, commitMessage.length) !== '.') {
          commitMessage += '.'
        }
        releaseNotes += `* ${commitMessage} #${prNumber} \n\n`
        prCount++
      }
    }
  })
  console.log(`${pass} Done generating release notes for ${currentBranch}. Found ${prCount} PRs.`)
  return releaseNotes
}

async function createRelease (branchToTarget, isBeta) {
  let releaseNotes = await getReleaseNotes(branchToTarget)
  let newVersion = getNewVersion()
  await tagRelease(newVersion)
  const githubOpts = {
    owner: 'electron',
    repo: 'electron'
  }
  console.log(`Checking for existing draft release.`)
  let releases = await github.repos.getReleases(githubOpts)
    .catch(err => {
      console.log('$fail} Could not get releases.  Error was', err)
    })
  let drafts = releases.data.filter(release => release.draft &&
    release.tag_name === newVersion)
  if (drafts.length > 0) {
    console.log(`${fail} Aborting because draft release for
      ${drafts[0].tag_name} already exists.`)
    process.exit(1)
  }
  console.log(`${pass} A draft release does not exist; creating one.`)
  githubOpts.draft = true
  githubOpts.name = `electron ${newVersion}`
  if (isBeta) {
    githubOpts.body = `Note: This is a beta release.  Please file new issues ` +
      `for any bugs you find in it.\n \n This release is published to npm ` +
      `under the beta tag and can be installed via npm install electron@beta, ` +
      `or npm i electron@${newVersion.substr(1)}.\n \n ${releaseNotes}`
    githubOpts.name = `${githubOpts.name}`
    githubOpts.prerelease = true
  } else {
    githubOpts.body = releaseNotes
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

async function pushRelease (branch) {
  let pushDetails = await GitProcess.exec(['push', 'origin', `HEAD:${branch}`, '--follow-tags'], gitDir)
  if (pushDetails.exitCode === 0) {
    console.log(`${pass} Successfully pushed the release.  Wait for ` +
      `release builds to finish before running "npm run release".`)
  } else {
    console.log(`${fail} Error pushing the release: ` +
        `${pushDetails.stderr}`)
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
  let checkoutDetails = await GitProcess.exec([ 'tag', '-a', '-m', version, version ], gitDir)
  if (checkoutDetails.exitCode === 0) {
    console.log(`${pass} Successfully tagged ${version}.`)
  } else {
    console.log(`${fail} Error tagging ${version}: ` +
      `${checkoutDetails.stderr}`)
    process.exit(1)
  }
}

async function verifyNewVersion () {
  let newVersion = getNewVersion(true)
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

async function prepareRelease (isBeta, notesOnly) {
  if (args.automaticRelease && (pkg.version.indexOf('beta') === -1 ||
      versionType !== 'beta')) {
    console.log(`${fail} Automatic release is only supported for beta releases`)
    process.exit(1)
  }
  let currentBranch
  if (args.branch) {
    currentBranch = args.branch
  } else {
    currentBranch = await getCurrentBranch(gitDir)
  }
  if (notesOnly) {
    let releaseNotes = await getReleaseNotes(currentBranch)
    console.log(`Draft release notes are: \n${releaseNotes}`)
  } else {
    await verifyNewVersion()
    await createRelease(currentBranch, isBeta)
    await pushRelease(currentBranch)
    await runReleaseBuilds(currentBranch)
  }
}

prepareRelease(!args.stable, args.notesOnly)
