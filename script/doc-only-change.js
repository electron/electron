const args = require('minimist')(process.argv.slice(2))
const { GitProcess } = require('dugite')
const octokit = require('@octokit/rest')()
const path = require('path')

const SOURCE_ROOT = path.normalize(path.dirname(__dirname))

async function checkIfDocOnlyChange () {
  if (args.prNumber || args.prBranch || args.prURL) {
    try {
      let pullRequestNumber = args.prNumber
      if (!pullRequestNumber || isNaN(pullRequestNumber)) {
        if (args.prBranch) {
          // AppVeyor doesn't provide a PR number for branch builds - figure it out from the branch
          const prsForBranch = await octokit.pulls.list({
            owner: 'electron',
            repo: 'electron',
            state: 'open',
            head: `electron:${args.prBranch}`
          })
          if (prsForBranch.data.length === 1) {
            pullRequestNumber = prsForBranch.data[0].number
          } else if (prsForBranch.data.length === 0 && args.commit) {
            // There are no pull requests for this branch, set pullRequestNumber to -1 to use commit instead
            pullRequestNumber = -1
          } else {
            // If there is more than one PR on a branch, just assume that this is more than a doc change
            process.exit(1)
          }
        } else if (args.prURL) {
          // CircleCI doesn't provide the PR number for branch builds, but it does provide the PR URL
          const pullRequestParts = args.prURL.split('/')
          pullRequestNumber = pullRequestParts[pullRequestParts.length - 1]
        }
      }

      let filesChanged
      if (pullRequestNumber === -1 && args.commit) {
        // There are no pull requests for this branch, but we do have a commit that can be used to get files
        const result = await GitProcess.exec(['diff-tree', '--no-commit-id', '--name-only', '-r', args.commit])
        if (result.exitCode !== 0) {
          console.log('Failed to find changed files', GitProcess.parseError(result.stderr))
          process.exit(-1)
        }
        filesChanged = result.stdout.split(/\r\n|\r|\n/g)
      } else {
        filesChanged = await findFilesByPRNumber(pullRequestNumber)
      }

      const nonDocChange = filesChanged.find((fileName) => {
        const fileDirs = fileName.split('/')
        if (fileDirs[0] !== 'docs' && fileDirs[0] !== '') {
          return true
        }
      })
      if (nonDocChange) {
        process.exit(1)
      } else {
        process.exit(0)
      }
    } catch (ex) {
      console.error('Error getting list of files changed: ', ex)
      process.exit(-1)
    }
  } else {
    console.error(`Check if only the docs were changed for a commit.
    Usage: doc-only-change.js --prNumber=PR_NUMBER || --prBranch=PR_BRANCH || --prURL=PR_URL || --commit=COMMIT_SHA`)
    process.exit(-1)
  }
}

async function findFilesByPRNumber (prNumber) {
  const filesChanged = await octokit.pulls.listFiles({
    owner: 'electron', repo: 'electron', pull_number: prNumber
  })
  return filesChanged.data.map((fileInfo) => {
    return fileInfo.filename
  })
}

checkIfDocOnlyChange()
