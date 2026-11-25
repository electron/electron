// Run with: node --test script/benchmarks/test/*.test.mts
import { BaseVersions } from '@electron/fiddle-core';

import assert from 'node:assert/strict';
import { describe, it } from 'node:test';

import {
  latestNightly,
  nightlyPublishedAt,
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

// A fetch that answers GitHub's release-by-tag API from `published`, keyed by
// tag, and records the requests.
function mockFetch(published: Record<string, string>) {
  const requests: { url: string; headers: Record<string, string> }[] = [];
  const fetchImpl = (async (url: string, init?: RequestInit) => {
    requests.push({ url, headers: (init?.headers ?? {}) as Record<string, string> });
    const tag = url.split('/').at(-1)!;
    if (!(tag in published)) return new Response('{"message":"Not Found"}', { status: 404 });
    return Response.json({ tag_name: tag, published_at: published[tag] });
  }) as typeof fetch;
  return { fetchImpl, requests };
}

const failingFetch = (async () => {
  throw new Error('network down');
}) as typeof fetch;

describe('resolveVersions', () => {
  const evening = (date: string) => Date.parse(`${date}T20:48:00Z`);
  const { fetchImpl } = mockFetch({
    'v47.0.0-nightly.20261029': '2026-10-29T17:00:00Z',
    'v47.0.0-nightly.20261028': '2026-10-28T17:00:00Z'
  });

  it('compares the latest nightly against the one before it', async () => {
    assert.deepEqual(await resolveVersions(versions, {}, evening('2026-10-29'), failingFetch), {
      candidate: '47.0.0-nightly.20261029',
      baseline: '47.0.0-nightly.20261028'
    });
  });

  it('uses the previous release on the major line for a release candidate', async () => {
    assert.deepEqual(await resolveVersions(versions, { candidate: '45.1.0' }), {
      candidate: '45.1.0',
      baseline: '45.0.2'
    });
  });

  it('keeps explicit versions', async () => {
    assert.deepEqual(await resolveVersions(versions, { candidate: '45.1.0', baseline: '44.3.2' }), {
      candidate: '45.1.0',
      baseline: '44.3.2'
    });
  });

  it('runs on the evening a nightly is published', async () => {
    const resolved = await resolveVersions(versions, { maxCandidateAgeHours: 25 }, evening('2026-10-29'), fetchImpl);
    assert.deepEqual(resolved, { candidate: '47.0.0-nightly.20261029', baseline: '47.0.0-nightly.20261028' });
  });

  it('skips the next day when there is no new nightly', async () => {
    const resolved = await resolveVersions(versions, { maxCandidateAgeHours: 25 }, evening('2026-10-30'), fetchImpl);
    assert.deepEqual(resolved, { skip: 'the latest nightly is 28 hours old' });
  });

  it('runs the day after a nightly published after the evening run', async () => {
    // 47.0.0-nightly.20261030 was published at 23:42 UTC on its release day,
    // after that evening's run had found only 20261029.
    const late = new BaseVersions([
      ...versions.versions.map((ver) => versions.getReleaseInfo(ver)!),
      { ...versions.getReleaseInfo('47.0.0-nightly.20261029')!, version: '47.0.0-nightly.20261030', date: '2026-10-30' }
    ]);
    const { fetchImpl: lateFetch } = mockFetch({
      'v47.0.0-nightly.20261030': '2026-10-30T23:42:00Z',
      'v47.0.0-nightly.20261029': '2026-10-29T17:00:00Z'
    });
    assert.deepEqual(await resolveVersions(versions, { maxCandidateAgeHours: 25 }, evening('2026-10-30'), lateFetch), {
      skip: 'the latest nightly is 28 hours old'
    });
    // By its release date it would be 45 hours old here and never run.
    assert.deepEqual(await resolveVersions(late, { maxCandidateAgeHours: 25 }, evening('2026-10-31'), lateFetch), {
      candidate: '47.0.0-nightly.20261030',
      baseline: '47.0.0-nightly.20261029'
    });
    assert.deepEqual(await resolveVersions(late, { maxCandidateAgeHours: 25 }, evening('2026-11-01'), lateFetch), {
      skip: 'the latest nightly is 45 hours old'
    });
  });

  it("falls back to the nightly's release date when GitHub cannot be asked", async () => {
    assert.equal(releaseAgeHours(versions, '47.0.0-nightly.20261029', evening('2026-10-29')), 20.8);
    const missing = mockFetch({}).fetchImpl;
    for (const fetchImpl of [failingFetch, missing]) {
      const runs = await resolveVersions(versions, { maxCandidateAgeHours: 25 }, evening('2026-10-29'), fetchImpl);
      assert.equal('skip' in runs, false);
      const skips = await resolveVersions(versions, { maxCandidateAgeHours: 25 }, evening('2026-10-30'), fetchImpl);
      assert.deepEqual(skips, { skip: 'the latest nightly is 45 hours old' });
    }
  });

  it('does not skip an explicit candidate', async () => {
    const resolved = await resolveVersions(
      versions,
      { candidate: '47.0.0-nightly.20261026', maxCandidateAgeHours: 25 },
      evening('2026-10-30'),
      failingFetch
    );
    assert.deepEqual(resolved, { candidate: '47.0.0-nightly.20261026', baseline: '46.0.0-nightly.20261005' });
  });
});

describe('nightlyPublishedAt', () => {
  it('reads the release by tag, with GITHUB_TOKEN when set', async () => {
    const { fetchImpl, requests } = mockFetch({ 'v47.0.0-nightly.20261029': '2026-10-29T17:00:00Z' });
    const saved = process.env.GITHUB_TOKEN;
    process.env.GITHUB_TOKEN = 'test-token';
    try {
      assert.equal(await nightlyPublishedAt('47.0.0-nightly.20261029', fetchImpl), Date.parse('2026-10-29T17:00:00Z'));
    } finally {
      if (saved === undefined) delete process.env.GITHUB_TOKEN;
      else process.env.GITHUB_TOKEN = saved;
    }
    assert.equal(
      requests[0].url,
      'https://api.github.com/repos/electron/nightlies/releases/tags/v47.0.0-nightly.20261029'
    );
    assert.equal(requests[0].headers.authorization, 'Bearer test-token');
  });

  it('fails for a missing release', async () => {
    await assert.rejects(nightlyPublishedAt('47.0.0-nightly.20261031', mockFetch({}).fetchImpl), /404/);
  });
});
