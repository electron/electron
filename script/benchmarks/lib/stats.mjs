// Small, dependency-free statistics for paired benchmark comparisons:
// median, Welch's t-test and the Mann-Whitney U test (both two-sided).

export function median(values) {
  if (values.length === 0) return NaN;
  const sorted = [...values].sort((a, b) => a - b);
  const mid = sorted.length >> 1;
  return sorted.length % 2 ? sorted[mid] : (sorted[mid - 1] + sorted[mid]) / 2;
}

export function mean(values) {
  return values.reduce((sum, value) => sum + value, 0) / values.length;
}

// Sample variance (n - 1 denominator).
export function variance(values) {
  const m = mean(values);
  return values.reduce((sum, value) => sum + (value - m) ** 2, 0) / (values.length - 1);
}

// Lanczos approximation, g = 7, n = 9.
const LANCZOS = [
  0.9999999999998099, 676.5203681218851, -1259.1392167224028, 771.3234287776531, -176.61503916999186,
  12.507343278686905, -0.13857109526572012, 9.984369578019572e-6, 1.5056327351493116e-7
];

function logGamma(x) {
  if (x < 0.5) return Math.log(Math.PI / Math.sin(Math.PI * x)) - logGamma(1 - x);
  x -= 1;
  let a = LANCZOS[0];
  const t = x + 7.5;
  for (let i = 1; i < 9; i++) a += LANCZOS[i] / (x + i);
  return 0.5 * Math.log(2 * Math.PI) + (x + 0.5) * Math.log(t) - t + Math.log(a);
}

// Continued fraction for the regularized incomplete beta (modified Lentz).
function betaContinuedFraction(x, a, b) {
  const TINY = 1e-300;
  let c = 1;
  let d = 1 - ((a + b) * x) / (a + 1);
  if (Math.abs(d) < TINY) d = TINY;
  d = 1 / d;
  let h = d;
  for (let m = 1; m <= 300; m++) {
    const m2 = 2 * m;
    let aa = (m * (b - m) * x) / ((a + m2 - 1) * (a + m2));
    d = 1 + aa * d;
    if (Math.abs(d) < TINY) d = TINY;
    c = 1 + aa / c;
    if (Math.abs(c) < TINY) c = TINY;
    d = 1 / d;
    h *= d * c;
    aa = (-(a + m) * (a + b + m) * x) / ((a + m2) * (a + m2 + 1));
    d = 1 + aa * d;
    if (Math.abs(d) < TINY) d = TINY;
    c = 1 + aa / c;
    if (Math.abs(c) < TINY) c = TINY;
    d = 1 / d;
    const delta = d * c;
    h *= delta;
    if (Math.abs(delta - 1) < 1e-14) break;
  }
  return h;
}

export function regularizedIncompleteBeta(x, a, b) {
  if (x <= 0) return 0;
  if (x >= 1) return 1;
  const front = Math.exp(logGamma(a + b) - logGamma(a) - logGamma(b) + a * Math.log(x) + b * Math.log(1 - x));
  if (x < (a + 1) / (a + b + 2)) return (front * betaContinuedFraction(x, a, b)) / a;
  return 1 - (front * betaContinuedFraction(1 - x, b, a)) / b;
}

// Two-sided p-value of Student's t distribution.
export function studentTTwoSidedP(t, df) {
  if (!Number.isFinite(t)) return 0;
  return regularizedIncompleteBeta(df / (df + t * t), df / 2, 0.5);
}

// Complementary error function (Numerical Recipes erfcc, |error| < 1.2e-7).
function erfc(x) {
  const z = Math.abs(x);
  const t = 1 / (1 + 0.5 * z);
  const r =
    t *
    Math.exp(
      -z * z -
        1.26551223 +
        t *
          (1.00002368 +
            t *
              (0.37409196 +
                t *
                  (0.09678418 +
                    t *
                      (-0.18628806 +
                        t * (0.27886807 + t * (-1.13520398 + t * (1.48851587 + t * (-0.82215223 + t * 0.17087277))))))))
    );
  return x >= 0 ? r : 2 - r;
}

export function welchTTest(a, b) {
  if (a.length < 2 || b.length < 2) return { t: NaN, df: NaN, p: NaN };
  const va = variance(a) / a.length;
  const vb = variance(b) / b.length;
  const diff = mean(a) - mean(b);
  if (va + vb === 0) return { t: diff === 0 ? 0 : Infinity, df: NaN, p: diff === 0 ? 1 : 0 };
  const t = diff / Math.sqrt(va + vb);
  const df = (va + vb) ** 2 / (va ** 2 / (a.length - 1) + vb ** 2 / (b.length - 1));
  return { t, df, p: studentTTwoSidedP(t, df) };
}

// Number of arrangements with U = u for sample sizes n and m (no ties).
function exactUCounts(n, m) {
  // counts[i][j] is the distribution of U for sizes i and j; built bottom-up
  // with the recurrence f(i, j, u) = f(i - 1, j, u - j) + f(i, j - 1, u).
  let previousRow = [];
  for (let i = 0; i <= n; i++) {
    const row = [];
    for (let j = 0; j <= m; j++) {
      const size = i * j + 1;
      const dist = new Array(size).fill(0);
      if (i === 0 || j === 0) {
        dist[0] = 1;
      } else {
        const left = previousRow[j];
        const down = row[j - 1];
        for (let u = 0; u < size; u++) {
          dist[u] = (u - j >= 0 && u - j < left.length ? left[u - j] : 0) + (u < down.length ? down[u] : 0);
        }
      }
      row.push(dist);
    }
    previousRow = row;
  }
  return previousRow[m];
}

export function mannWhitneyU(a, b) {
  const n = a.length;
  const m = b.length;
  if (n === 0 || m === 0) return { u: NaN, p: NaN, exact: false };
  const combined = [...a.map((value) => ({ value, group: 0 })), ...b.map((value) => ({ value, group: 1 }))];
  combined.sort((x, y) => x.value - y.value);
  const ranks = new Array(combined.length);
  let tieCorrection = 0;
  for (let i = 0; i < combined.length; ) {
    let j = i;
    while (j + 1 < combined.length && combined[j + 1].value === combined[i].value) j++;
    const rank = (i + j) / 2 + 1;
    for (let k = i; k <= j; k++) ranks[k] = rank;
    const ties = j - i + 1;
    tieCorrection += ties ** 3 - ties;
    i = j + 1;
  }
  let rankSumA = 0;
  combined.forEach((entry, i) => {
    if (entry.group === 0) rankSumA += ranks[i];
  });
  const u = rankSumA - (n * (n + 1)) / 2;
  const mu = (n * m) / 2;

  // Exact distribution for small samples without ties.
  if (tieCorrection === 0 && n * m <= 2500) {
    const counts = exactUCounts(n, m);
    const total = counts.reduce((sum, c) => sum + c, 0);
    const lower = Math.min(u, n * m - u);
    let tail = 0;
    for (let k = 0; k <= lower; k++) tail += counts[k];
    return { u, p: Math.min(1, (2 * tail) / total), exact: true };
  }

  // Normal approximation with tie and continuity corrections.
  const total = n + m;
  const sigma = Math.sqrt(((n * m) / 12) * (total + 1 - tieCorrection / (total * (total - 1))));
  if (sigma === 0) return { u, p: 1, exact: false };
  const z = Math.max(0, Math.abs(u - mu) - 0.5) / sigma;
  return { u, p: Math.min(1, erfc(z / Math.SQRT2)), exact: false };
}

// Compares two samples of one metric. `lowerIsBetter` decides which direction
// of change counts as a regression. A change is only flagged when both tests
// agree (p < alpha) and the relative change of the medians is at least
// minChange (0.10 = 10%).
export function compareSamples(reference, candidate, { lowerIsBetter = true, alpha = 0.05, minChange = 0.1 } = {}) {
  const referenceMedian = median(reference);
  const candidateMedian = median(candidate);
  const relativeChange = referenceMedian === 0 ? NaN : (candidateMedian - referenceMedian) / Math.abs(referenceMedian);
  const welch = welchTTest(candidate, reference);
  const mwu = mannWhitneyU(candidate, reference);
  const significant = welch.p < alpha && mwu.p < alpha && Math.abs(relativeChange) >= minChange;
  let verdict = 'no change';
  if (reference.length < 2 || candidate.length < 2) {
    verdict = 'insufficient data';
  } else if (significant) {
    const worse = lowerIsBetter ? relativeChange > 0 : relativeChange < 0;
    verdict = worse ? 'regression' : 'improvement';
  }
  return {
    referenceMedian,
    candidateMedian,
    relativeChange,
    welchP: welch.p,
    mannWhitneyP: mwu.p,
    referenceCount: reference.length,
    candidateCount: candidate.length,
    verdict
  };
}
