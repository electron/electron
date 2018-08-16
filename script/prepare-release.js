#!/usr/bin/env node

if (!process.env.CI) require('dotenv-safe').load()
require('colors')
const args = require('minimist')(process.argv.slice(2), {
  boolean: ['automaticRelease', 'notesOnly', 'stable']
})
const ciReleaseBuild = require('./ci-release-build')
const { execSync } = require('child_process')
const fail = '\u2717'.red
const { GitProcess } = require('dugite')
const GitHub = require('github')
const pass = '\u2713'.green
const path = require('path')
const pkg = require('../package.json')
const readline = require('readline')
const semver = require('semver')
const versionType = args._[0]
const targetRepo = versionType === 'nightly' ? 'nightlies' : 'electron'

// TODO (future) automatically determine version based on conventional commits
// via conventional-recommended-bump

if (!versionType && !args.notesOnly) {
  console.log(`Usage: prepare-release versionType [major | minor | patch | beta | nightly]` +
     ` (--stable) (--notesOnly) (--automaticRelease) (--branch)`)
  process.exit(1)
}

const github = new GitHub()
const gitDir = path.resolve(__dirname, '..')
github.authenticate({type: 'token', token: process.env.ELECTRON_GITHUB_TOKEN})

async function getNewVersion (dryRun) {
  console.log(`Bumping for new "${versionType}" version.`)
  let bumpScript = path.join(__dirname, 'bump-version.py')
  let scriptArgs = [bumpScript]
  if (versionType === 'nightly') {
    scriptArgs.push(`--version ${await determineNextNightly(await getCurrentBranch())}`)
  } else {
    scriptArgs.push(`--bump ${versionType}`)
  }
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
    throw err
  }
}

async function determineNextNightly (currentBranch) {
  const twoPad = (n) => n < 10 ? `0${n}` : `${n}`
  const d = new Date()
  const date = `${d.getFullYear()}${twoPad(d.getMonth() + 1)}${twoPad(d.getDate())}`

  let version

  if (currentBranch === 'master') {
    version = await determineNextNightlyForMaster()
  }
  if (!version) {
    throw new Error(`not yet implemented for release branch: ${currentBranch}`)
  }

  return `${version}-nightly.${date}`
}

async function determineNextNightlyForMaster () {
  let branchNames
  let result = await GitProcess.exec(['branch', '-a', '--remote', '--list', 'origin/[0-9]-[0-9]-x'], gitDir)
  if (result.exitCode === 0) {
    branchNames = result.stdout.trim().split('\n')
    const filtered = branchNames.map(b => b.replace('origin/', ''))
    return getNextReleaseBranch(filtered)
  } else {
    throw new Error('Release branches could not be fetched.')
  }
}

function getNextReleaseBranch (branches) {
  const converted = branches.map(b => b.replace(/-/g, '.').replace('x', '0'))
  const next = converted.reduce((v1, v2) => {
    return semver.gt(v1, v2) ? v1 : v2
  })
  return `${parseInt(next.split('.')[0], 10) + 1}.0.0`
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
  if (versionType === 'nightly') {
    return 'Nightlies do not get release notes, please compare tags for info'
  }
  console.log(`Generating release notes for ${currentBranch}.`)
  let githubOpts = {
    owner: 'electron',
    repo: targetRepo,
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
  let newVersion = await getNewVersion()
  await tagRelease(newVersion)
  const githubOpts = {
    owner: 'electron',
    repo: targetRepo
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
    if (versionType === 'nightly') {
      githubOpts.body = `Note: This is a beta release.  Please file new issues ` +
        `for any bugs you find in it.\n \n This release is published to npm ` +
        `under the beta tag and can be installed via npm install electron@beta, ` +
        `or npm i electron@${newVersion.substr(1)}.\n \n ${releaseNotes}`
    } else {
      githubOpts.body = `Note: This is a nightly release.  Please file new issues ` +
        `for any bugs you find in it.\n \n This release is published to npm ` +
        `under the nightly tag and can be installed via npm install electron@nightly, ` +
        `or npm i electron@${newVersion.substr(1)}.\n \n ${releaseNotes}`
    }
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
  let newVersion = await getNewVersion(true)
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
      versionType !== 'beta') && versionType !== 'nightly') {
    console.log(`${fail} Automatic release is only supported for beta and nightly releases`)
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
