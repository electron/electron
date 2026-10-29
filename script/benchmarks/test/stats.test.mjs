// Run with: node --test script/benchmarks/test/
// Reference values come from scipy.stats (ttest_ind with equal_var=False and
// mannwhitneyu with alternative='two-sided').
import assert from 'node:assert/strict';
import { describe, it } from 'node:test';

import { compareSamples, mannWhitneyU, median, welchTTest } from '../lib/stats.mjs';

const close = (actual, expected, tolerance = 1e-4) =>
  assert.ok(Math.abs(actual - expected) <= tolerance, `expected ${actual} to be within ${tolerance} of ${expected}`);

describe('median', () => {
  it('handles odd and even lengths', () => {
    assert.equal(median([3, 1, 2]), 2);
    assert.equal(median([4, 1, 3, 2]), 2.5);
    assert.ok(Number.isNaN(median([])));
  });
});

describe('welchTTest', () => {
  it('matches scipy for unequal variances', () => {
    const { t, df, p } = welchTTest(
      [19.8, 20.4, 19.6, 17.8, 18.5, 18.9, 18.3, 18.9, 19.5, 22.0],
      [28.2, 26.6, 20.1, 23.3, 25.2, 22.1, 17.7, 27.6, 20.6, 13.7]
    );
    close(t, -2.074015, 1e-5);
    close(df, 10.209185, 1e-4);
    close(p, 0.06428, 1e-5);
  });

  it('returns p = 1 for identical constant samples', () => {
    assert.equal(welchTTest([5, 5, 5], [5, 5, 5]).p, 1);
  });
});

describe('mannWhitneyU', () => {
  it('uses the exact distribution for small samples without ties', () => {
    const { u, p, exact } = mannWhitneyU([1, 2, 3], [4, 5, 6]);
    assert.equal(u, 0);
    assert.ok(exact);
    close(p, 0.1);
  });

  it('matches scipy for a larger exact case', () => {
    const { p } = mannWhitneyU([1.1, 2.3, 3.5, 4.2, 5.9, 6.1, 7.4, 8.8], [3.3, 5.1, 6.6, 7.9, 9.2, 10.4, 11.7, 12.5]);
    close(p, 0.049883, 1e-5);
  });

  it('falls back to the normal approximation with ties', () => {
    const { p, exact } = mannWhitneyU([1, 2, 2, 3, 4, 5], [2, 3, 5, 6, 7, 8]);
    assert.equal(exact, false);
    close(p, 0.089269, 1e-5);
  });
});

describe('compareSamples', () => {
  const baseline = [100, 102, 99, 101, 100, 98, 103, 100, 101, 99];

  it('flags a significant slowdown above the minimum change as a regression', () => {
    const result = compareSamples(
      baseline,
      baseline.map((v) => v * 1.2),
      { lowerIsBetter: true }
    );
    assert.equal(result.verdict, 'regression');
    close(result.relativeChange, 0.2, 1e-9);
  });

  it('treats a higher score as an improvement when higher is better', () => {
    const result = compareSamples(
      baseline,
      baseline.map((v) => v * 1.2),
      { lowerIsBetter: false }
    );
    assert.equal(result.verdict, 'improvement');
  });

  it('ignores significant changes below the minimum change', () => {
    const result = compareSamples(
      baseline,
      baseline.map((v) => v * 1.05)
    );
    assert.ok(result.welchP < 0.05);
    assert.equal(result.verdict, 'no change');
  });

  it('ignores large changes that are not significant', () => {
    assert.equal(compareSamples([100, 300], [150, 350]).verdict, 'no change');
  });

  it('reports insufficient data', () => {
    assert.equal(compareSamples([100], [200]).verdict, 'insufficient data');
  });
});
