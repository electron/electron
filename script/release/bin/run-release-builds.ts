import { parseArgs } from 'node:util';

import { runReleaseCIJobs } from '../run-release-ci-jobs';

const { values: { ghRelease, job, arch, ci, commit, newVersion }, positionals } = parseArgs({
  options: {
    ghRelease: {
      type: 'boolean'
    },
    job: {
      type: 'string'
    },
    arch: {
      type: 'string'
    },
    ci: {
      type: 'string'
    },
    commit: {
      type: 'string'
    },
    newVersion: {
      type: 'string'
    }
  },
  allowPositionals: true
});

const targetBranch = positionals[0];

if (positionals.length < 1) {
  console.log(`Trigger CI to build release builds of electron.
  Usage: ci-release-build.js [--job=CI_JOB_NAME] [--arch=INDIVIDUAL_ARCH] [--ci=AppVeyor|GitHubActions]
  [--ghRelease] [--commit=sha] [--newVersion=version_tag] TARGET_BRANCH
  `);
  process.exit(0);
}

if (ci === 'GitHubActions' || !ci) {
  if (!newVersion) {
    console.error('--newVersion is required for GitHubActions');
    process.exit(1);
  }
}

runReleaseCIJobs(targetBranch, {
  ci: ci as 'GitHubActions' | 'AppVeyor',
  ghRelease,
  job: job as any,
  arch,
  newVersion: newVersion!,
  commit
});
