// Run with: node --test script/benchmarks/test/*.test.mts
import { BaseVersions } from '@electron/fiddle-core';

import assert from 'node:assert/strict';
import { describe, it } from 'node:test';

import {
  latestNightly,
  previousNightly,
  previousRelease,
  releaseAgeHours,
  resolveVersions
} from '../fetch-targets.mts';

// Newest first by date, as in releases.json: several lines ship together.
const RELEASES: [string, string][] = [
  ['47.0.0-nightly.20261029', '2026-10-29'],
  ['47.0.0-nightly.20261028', '2026-10-28'],
  ['46.0.0-beta.4', '2026-10-28'],
  ['45.1.0', '2026-10-27'],
  ['44.3.2', '2026-10-27'],
  ['47.0.0-nightly.20261026', '2026-10-26'],
  ['45.0.2', '2026-10-20'],
  ['46.0.0-beta.3', '2026-10-20'],
  ['45.0.1', '2026-10-13'],
  ['46.0.0-alpha.1', '2026-10-06'],
  ['46.0.0-nightly.20261005', '2026-10-05'],
  ['45.0.0', '2026-09-01'],
  ['45.0.0-beta.3', '2026-08-25'],
  ['45.0.0-beta.2', '2026-08-18'],
  ['44.3.1', '2026-08-18'],
  ['45.0.0-alpha.2', '2026-08-04'],
  ['45.0.0-alpha.1', '2026-07-28'],
  ['45.0.0-nightly.20260601', '2026-06-01']
];

const versions = new BaseVersions(
  RELEASES.map(([version, date]) => ({
    version,
    date,
    node: '24.0.0',
    v8: '15.0',
    uv: '1.0',
    zlib: '1.0',
    openssl: '0.0.0',
    modules: '154',
    chrome: `${100 + Number(version.split('.')[0])}.0.1.0`,
    files: ['linux-x64']
  }))
);

describe('previousRelease', () => {
  it('stays on the candidate major line', () => {
    assert.equal(previousRelease(versions, '45.1.0'), '45.0.2');
    assert.equal(previousRelease(versions, '44.3.2'), '44.3.1');
    assert.equal(previousRelease(versions, '46.0.0-beta.4'), '46.0.0-beta.3');
  });

  it('steps back through prereleases, skipping nightlies', () => {
    assert.equal(previousRelease(versions, '45.0.0-beta.3'), '45.0.0-beta.2');
    assert.equal(previousRelease(versions, '45.0.0-beta.2'), '45.0.0-alpha.2');
    assert.equal(previousRelease(versions, '45.0.0'), '45.0.0-beta.3');
  });

  it('works for a candidate missing from releases.json', () => {
    assert.equal(previousRelease(versions, '45.1.1'), '45.1.0');
  });

  it('fails when the major line has no older release', () => {
    assert.throws(() => previousRelease(versions, '46.0.0-alpha.1'), /No 46\.x release before 46\.0\.0-alpha\.1/);
    assert.throws(() => previousRelease(new BaseVersions([]), '45.1.0'), /No 45\.x release before 45\.1\.0/);
  });
});

describe('nightlies', () => {
  it('finds the latest nightly', () => {
    assert.equal(latestNightly(versions), '47.0.0-nightly.20261029');
  });

  it('finds the previous nightly, also across major lines', () => {
    assert.equal(previousNightly(versions, '47.0.0-nightly.20261029'), '47.0.0-nightly.20261028');
    assert.equal(previousNightly(versions, '47.0.0-nightly.20261026'), '46.0.0-nightly.20261005');
  });

  it('finds the previous nightly of one newer than releases.json', () => {
    assert.equal(previousNightly(versions, '47.0.0-nightly.20261030'), '47.0.0-nightly.20261029');
  });

  it('fails for the oldest nightly', () => {
    assert.throws(() => previousNightly(versions, '45.0.0-nightly.20260601'), /Cannot find the nightly before/);
  });
});

describe('resolveVersions', () => {
  const evening = (date: string) => Date.parse(`${date}T20:43:00Z`);

  it('compares the latest nightly against the one before it', () => {
    assert.deepEqual(resolveVersions(versions, {}, evening('2026-10-29')), {
      candidate: '47.0.0-nightly.20261029',
      baseline: '47.0.0-nightly.20261028'
    });
  });

  it('uses the previous release on the major line for a release candidate', () => {
    assert.deepEqual(resolveVersions(versions, { candidate: '45.1.0' }), { candidate: '45.1.0', baseline: '45.0.2' });
  });

  it('keeps explicit versions', () => {
    assert.deepEqual(resolveVersions(versions, { candidate: '45.1.0', baseline: '44.3.2' }), {
      candidate: '45.1.0',
      baseline: '44.3.2'
    });
  });

  it("runs on the evening of a nightly's release day", () => {
    assert.equal(releaseAgeHours(versions, '47.0.0-nightly.20261029', evening('2026-10-29')), 20.716666666666665);
    const resolved = resolveVersions(versions, { maxCandidateAgeHours: 24 }, evening('2026-10-29'));
    assert.equal('skip' in resolved, false);
  });

  it('skips a day without a new nightly', () => {
    const resolved = resolveVersions(versions, { maxCandidateAgeHours: 24 }, evening('2026-10-30'));
    assert.deepEqual(resolved, { skip: 'the latest nightly is 45 hours old' });
  });

  it('does not skip an explicit candidate', () => {
    const resolved = resolveVersions(
      versions,
      { candidate: '47.0.0-nightly.20261026', maxCandidateAgeHours: 24 },
      evening('2026-10-30')
    );
    assert.deepEqual(resolved, { candidate: '47.0.0-nightly.20261026', baseline: '46.0.0-nightly.20261005' });
  });
});
