const args = require('minimist')(process.argv.slice(2));
const { Octokit } = require('@octokit/rest');
const octokit = new Octokit();

async function checkIfDocOnlyChange () {
  let { prNumber, prURL } = args;

  if (prNumber || prURL) {
    try {
      // CircleCI doesn't provide the PR number except on forked PRs,
      // so to cover all cases we just extract it from the PR URL.
      if (!prNumber || isNaN(prNumber)) {
        if (args.prURL) {
          prNumber = prURL.split('/').pop();
        }
      }

      const filesChanged = await octokit.paginate(octokit.pulls.listFiles.endpoint.merge({
        owner: 'electron',
        repo: 'electron',
        pull_number: prNumber,
        per_page: 100
      }));

      console.log('Changed Files: ', filesChanged.map(fileInfo => fileInfo.filename));

      const nonDocChange = filesChanged.length === 0 || filesChanged.find(({ filename }) => {
        const fileDirs = filename.split('/');
        if (fileDirs[0] !== 'docs') return true;
      });

      process.exit(nonDocChange ? 1 : 0);
    } catch (error) {
      console.error('Error getting list of files changed: ', error);
      process.exit(-1);
    }
  } else {
    console.error(`Check if only the docs were changed for a commit.
    Usage: doc-only-change.js --prNumber=PR_NUMBER || --prURL=PR_URL`);
    process.exit(-1);
  }
}

checkIfDocOnlyChange();
