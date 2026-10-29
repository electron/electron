// Run with: node --test script/benchmarks/test/
import assert from 'node:assert/strict';
import { afterEach, describe, it } from 'node:test';

import { compareSemver, listNightlies, previousNightly, previousRelease } from '../fetch-targets.mjs';

describe('compareSemver', () => {
  it('orders prereleases before their release', () => {
    const sorted = ['45.0.0', '45.0.0-beta.10', '44.2.1', '45.0.0-alpha.3', '45.0.0-beta.2', '45.0.1'].sort(
      compareSemver
    );
    assert.deepEqual(sorted, ['44.2.1', '45.0.0-alpha.3', '45.0.0-beta.2', '45.0.0-beta.10', '45.0.0', '45.0.1']);
  });
});

describe('previousRelease', () => {
  // Newest first by date, as in releases.json: several lines ship together.
  const releases = [
    '47.0.0-nightly.20261028',
    '46.0.0-beta.4',
    '45.1.0',
    '44.3.2',
    '45.0.2',
    '46.0.0-beta.3',
    '45.0.1',
    '46.0.0-alpha.1',
    '45.0.0',
    '45.0.0-beta.3',
    '45.0.0-beta.2',
    '44.3.1',
    '45.0.0-alpha.2',
    '45.0.0-alpha.1',
    '45.0.0-nightly.20260601'
  ].map((version) => ({ version }));

  it('stays on the candidate major line', () => {
    assert.equal(previousRelease(releases, '45.1.0'), '45.0.2');
    assert.equal(previousRelease(releases, '44.3.2'), '44.3.1');
    assert.equal(previousRelease(releases, '46.0.0-beta.4'), '46.0.0-beta.3');
  });

  it('steps back through prereleases', () => {
    assert.equal(previousRelease(releases, '45.0.0-beta.3'), '45.0.0-beta.2');
    assert.equal(previousRelease(releases, '45.0.0-beta.2'), '45.0.0-alpha.2');
    assert.equal(previousRelease(releases, '45.0.0'), '45.0.0-beta.3');
  });

  it('works for a candidate missing from releases.json', () => {
    assert.equal(previousRelease(releases, '45.1.1'), '45.1.0');
  });

  it('fails when the major line has no older release', () => {
    assert.throws(() => previousRelease(releases, '46.0.0-alpha.1'), /No 46\.x release before 46\.0\.0-alpha\.1/);
    assert.throws(() => previousRelease([], '45.1.0'), /No 45\.x release before 45\.1\.0/);
  });
});

describe('nightlies', () => {
  const realFetch = globalThis.fetch;
  afterEach(() => {
    globalThis.fetch = realFetch;
  });

  // 250 nightlies, one a day, newest first, served 100 per page.
  const day = 24 * 3_600_000;
  const start = Date.parse('2026-10-28T16:00:00Z');
  const all = Array.from({ length: 250 }, (_, i) => {
    const date = new Date(start - i * day);
    const stamp = date.toISOString().slice(0, 10).replaceAll('-', '');
    return { tag_name: `v47.0.0-nightly.${stamp}`, draft: false, published_at: date.toISOString() };
  });
  const versionAt = (i) => all[i].tag_name.slice(1);

  function mockFetch() {
    const pages = [];
    globalThis.fetch = async (url) => {
      const { searchParams } = new URL(url);
      const page = Number(searchParams.get('page'));
      const perPage = Number(searchParams.get('per_page'));
      pages.push(page);
      return new Response(JSON.stringify(all.slice((page - 1) * perPage, page * perPage)), { status: 200 });
    };
    return pages;
  }

  it('reads one page for the latest nightly', async () => {
    const pages = mockFetch();
    const [latest] = await listNightlies();
    assert.equal(latest.version, versionAt(0));
    assert.deepEqual(pages, [1]);
  });

  it('pages back to find an older nightly and its predecessor', async () => {
    const pages = mockFetch();
    assert.equal(await previousNightly(versionAt(150)), versionAt(151));
    assert.deepEqual(pages, [1, 2]);
  });

  it('fetches the next page when the predecessor is across a page boundary', async () => {
    const pages = mockFetch();
    assert.equal(await previousNightly(versionAt(99)), versionAt(100));
    assert.deepEqual(pages, [1, 2]);
  });

  it('fails for the oldest nightly or an unknown one', async () => {
    let pages = mockFetch();
    await assert.rejects(previousNightly(versionAt(249)), /Cannot find the nightly before/);
    assert.deepEqual(pages, [1, 2, 3]);
    pages = mockFetch();
    await assert.rejects(previousNightly('40.0.0-nightly.20250101'), /Cannot find the nightly before/);
    assert.deepEqual(pages, [1, 2, 3]);
  });

  it('stops at the page cap', async () => {
    const pages = mockFetch();
    const list = await listNightlies({ done: () => false, maxPages: 2 });
    assert.equal(list.length, 200);
    assert.deepEqual(pages, [1, 2]);
  });
});
