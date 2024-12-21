import { parseArgs } from 'node:util';

import { cleanReleaseArtifacts } from '../release-artifact-cleanup';

const { values: { tag: _tag, releaseID } } = parseArgs({
  options: {
    tag: {
      type: 'string'
    },
    releaseID: {
      type: 'string',
      default: ''
    }
  }
});

if (!_tag) {
  console.error('Missing --tag argument');
  process.exit(1);
}

const tag = _tag;

cleanReleaseArtifacts({
  releaseID,
  tag
})
  .catch((err) => {
    console.error(err);
    process.exit(1);
  });
