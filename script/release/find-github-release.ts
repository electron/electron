import { Octokit } from '@octokit/rest';

import { createGitHubTokenStrategy } from './github-token';
import { ELECTRON_ORG, ELECTRON_REPO, ElectronReleaseRepo, NIGHTLY_REPO } from './types';

if (process.argv.length < 3) {
  console.log('Usage: find-release version');
  process.exit(1);
}

const version = process.argv[2];
const targetRepo = findRepo();

const octokit = new Octokit({
  authStrategy: createGitHubTokenStrategy(targetRepo)
});

function findRepo (): ElectronReleaseRepo {
  return version.indexOf('nightly') > 0 ? NIGHTLY_REPO : ELECTRON_REPO;
}

async function findRelease () {
  const releases = await octokit.repos.listReleases({
    owner: ELECTRON_ORG,
    repo: targetRepo
  });

  const targetRelease = releases.data.find(release => release.tag_name === version);
  let returnObject = {};

  if (targetRelease) {
    returnObject = {
      id: targetRelease.id,
      draft: targetRelease.draft,
      exists: true
    };
  } else {
    returnObject = {
      exists: false,
      draft: false
    };
  }
  console.log(JSON.stringify(returnObject));
}

findRelease()
  .catch((err) => {
    console.error(err);
    process.exit(1);
  });
