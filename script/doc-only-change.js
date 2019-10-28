const args = require('minimist')(process.argv.slice(2))
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
          } else {
            // If there are 0 PRs or more than one PR on a branch, just assume that this is more than a doc change
            process.exit(1)
          }
        } else if (args.prURL) {
          // CircleCI doesn't provide the PR number for branch builds, but it does provide the PR URL
          const pullRequestParts = args.prURL.split('/')
          pullRequestNumber = pullRequestParts[pullRequestParts.length - 1]
        }
      }
      const filesChanged = await octokit.pulls.listFiles({
        owner: 'electron', repo: 'electron', pull_number: pullRequestNumber
      })

      const nonDocChange = filesChanged.data.find((fileInfo) => {
        const fileDirs = fileInfo.filename.split('/')
        if (fileDirs[0] !== 'docs') {
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
    Usage: doc-only-change.js --prNumber=PR_NUMBER || --prBranch=PR_BRANCH || --prURL=PR_URL`)
    process.exit(-1)
  }
}

checkIfDocOnlyChange()
