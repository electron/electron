// Run with: node --test script/benchmarks/test/*.test.mts
import { BaseVersions } from '@electron/fiddle-core';

import assert from 'node:assert/strict';
import { describe, it } from 'node:test';

import { DEFAULT_ANCHOR, latestStableInMajor, parseMajors, resolveHistory } from '../resolve-history.mts';

const versions = new BaseVersions(
  [
    '47.0.0-nightly.20261029',
    '46.0.0-beta.4',
    '46.0.0-alpha.1',
    '45.3.1',
    '45.3.0',
    '45.2.3',
    '45.2.2',
    '45.0.0',
    '45.0.0-beta.3',
    '44.11.0',
    '44.3.2',
    '30.5.1',
    '30.0.0',
    '29.0.0-beta.1'
  ].map((version) => ({
    version,
    date: '2026-10-29',
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

describe('parseMajors', () => {
  it('expands ranges and lists', () => {
    assert.deepEqual(parseMajors('30-33', 45), [30, 31, 32, 33]);
    assert.deepEqual(parseMajors('44, 30,30-31', 45), [30, 31, 44]);
  });

  it('expands "latest" to the newest stable major', () => {
    assert.deepEqual(parseMajors('43-latest', 45), [43, 44, 45]);
    assert.deepEqual(parseMajors('latest', 45), [45]);
  });

  it('rejects malformed input', () => {
    assert.throws(() => parseMajors('30-', 45), /Invalid major/);
    assert.throws(() => parseMajors('33-30', 45), /before/);
    assert.throws(() => parseMajors('v30', 45), /Invalid major/);
    assert.throws(() => parseMajors('1-2-3', 45), /Invalid range/);
    assert.throws(() => parseMajors(' , ', 45), /No majors/);
  });
});

describe('latestStableInMajor', () => {
  it('skips prereleases', () => {
    assert.equal(latestStableInMajor(versions, 45), '45.3.1');
    assert.equal(latestStableInMajor(versions, 44), '44.11.0');
    assert.equal(latestStableInMajor(versions, 46), undefined);
    assert.equal(latestStableInMajor(versions, 29), undefined);
  });
});

describe('resolveHistory', () => {
  it('resolves every major to its latest stable release and the anchor to the pinned default', () => {
    assert.equal(DEFAULT_ANCHOR, '45.3.0');
    assert.deepEqual(resolveHistory(versions, { majors: '29-latest' }), {
      anchor: '45.3.0',
      jobs: [
        { major: 30, version: '30.5.1' },
        { major: 44, version: '44.11.0' },
        { major: 45, version: '45.3.1' }
      ],
      missing: [29, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43]
    });
  });

  it('uses the default anchor for an empty one', () => {
    assert.equal(resolveHistory(versions, { majors: '30', anchor: '' }).anchor, '45.3.0');
  });

  it('accepts latest-stable or an explicit anchor', () => {
    assert.equal(resolveHistory(versions, { majors: '30', anchor: 'latest-stable' }).anchor, '45.3.1');
    assert.equal(resolveHistory(versions, { majors: '30', anchor: '44.3.2' }).anchor, '44.3.2');
    assert.throws(() => resolveHistory(versions, { majors: '30', anchor: '44.0.0' }), /Unknown anchor/);
  });

  it('fails when no major has a stable release', () => {
    assert.throws(() => resolveHistory(versions, { majors: '46-47' }), /None of the majors/);
  });
});
