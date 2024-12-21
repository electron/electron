const { Octokit } = require('@octokit/rest');
const minimist = require('minimist');

const args = minimist(process.argv.slice(2));

const octokit = new Octokit();

async function checkIfDocOnlyChange () {
  let { prNumber, prURL } = args;

  if (prNumber || prURL) {
    try {
      // extract the PR number from the PR URL.
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
        return false;
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
