import { parseArgs } from 'node:util';

import { printNextVersion } from '../prepare-release';
import { ELECTRON_REPO, isVersionBumpType, NIGHTLY_REPO } from '../types';

const { values: { branch }, positionals } = parseArgs({
  options: {
    branch: {
      type: 'string'
    }
  },
  allowPositionals: true
});

const bumpType = positionals[0];

if (!bumpType || !isVersionBumpType(bumpType)) {
  console.log('Usage: print-next-version [stable | minor | beta | alpha | nightly]' +
     ' (--branch=branch)');
  process.exit(1);
}

printNextVersion({
  isPreRelease: bumpType !== 'stable' && bumpType !== 'minor',
  targetRepo: bumpType === 'nightly' ? NIGHTLY_REPO : ELECTRON_REPO,
  targetBranch: branch,
  bumpType
})
  .catch((err) => {
    console.error(err);
    process.exit(1);
  });
