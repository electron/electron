// Compares the targets of one interleaved benchmark run (see run.mjs).
//
// For every metric it takes one sample per round and target (the median of
// that round's repetitions), then compares the candidate against the baseline
// and against Chrome for Testing with Welch's t-test and the Mann-Whitney U
// test. A change is flagged only when both tests give p < alpha AND the
// medians differ by at least --min-change.
//
// Writes <results>/comparison.json and a Markdown table (to
// $GITHUB_STEP_SUMMARY when set), and uploads to Datadog when DD_API_KEY is
// set and --dry-run is not passed:
//   electron.perf.<metric>        median per target
//   electron.perf.delta.<metric>  relative change in percent per comparison
//
// Usage:
//   node script/benchmarks/compare.mjs --results <dir> [--min-change 0.1]
//     [--alpha 0.05] [--dry-run] [--fail-on-regression]
import * as fs from 'node:fs';
import * as path from 'node:path';
import { fileURLToPath } from 'node:url';
import { parseArgs } from 'node:util';

import { compareSamples, median } from './lib/stats.mjs';

// Measures reported per story, from the js probe (loading suites) or the
// benchmark's own result file (press benchmarks).
function measuresFor(suite, story) {
  if (suite === 'file_loading') return ['files_loaded_ms'];
  if (story === 'resize') return ['resize_median_ms', 'resize_p90_ms'];
  return ['fcp', 'lcp', 'load_event_end'];
}
const PRESS_RESULTS = {
  speedometer: { file: 'speedometer_3.1.json', keys: ['Score'] },
  jetstream: { file: 'jetstream_2.2.json', keys: ['Total/Score'] }
};

const COMPARISONS = [
  { name: 'vs_baseline', reference: 'baseline', candidate: 'candidate', title: 'Candidate vs baseline' },
  { name: 'vs_cft', reference: 'cft', candidate: 'candidate', title: 'Electron vs Chrome for Testing (same milestone)' }
];

const readJson = (file) => JSON.parse(fs.readFileSync(file, 'utf8'));

// Values of `data[key].values`, as written by crossbench's metrics merger.
const valuesOf = (entry) => (Array.isArray(entry?.values) ? entry.values.filter(Number.isFinite) : []);

// Returns { metric: number } for one crossbench invocation.
function collectInvocation(resultDir, suite) {
  const metrics = {};
  if (!fs.existsSync(resultDir)) return metrics;

  if (PRESS_RESULTS[suite]) {
    const file = path.join(resultDir, PRESS_RESULTS[suite].file);
    if (!fs.existsSync(file)) return metrics;
    for (const browser of Object.values(readJson(file))) {
      const key = PRESS_RESULTS[suite].keys.find((k) => valuesOf(browser.data?.[k]).length > 0);
      if (key) metrics[`${suite}.score`] = median(valuesOf(browser.data[key]));
    }
    return metrics;
  }

  // Each invocation runs one browser, so there is one <browser>/stories dir.
  for (const browserDir of fs.readdirSync(resultDir)) {
    const storiesDir = path.join(resultDir, browserDir, 'stories');
    if (!fs.existsSync(storiesDir) || !fs.statSync(storiesDir).isDirectory()) continue;
    for (const story of fs.readdirSync(storiesDir)) {
      const file = path.join(storiesDir, story, 'js.json');
      if (!fs.existsSync(file)) continue;
      const data = readJson(file);
      for (const measure of measuresFor(suite, story)) {
        const values = valuesOf(data[measure]);
        if (values.length) metrics[`${suite}.${story}.${measure}`] = median(values);
      }
    }
  }

  // Alarm on the cost of each custom-protocol strategy relative to file://
  // in the same round, not only on absolute times.
  if (suite === 'file_loading') {
    for (const [metric, value] of Object.entries(metrics)) {
      const [, story] = metric.split('.');
      const kind = story.split('_').pop();
      const reference = metrics[`file_loading.file_${kind}.files_loaded_ms`];
      if (!story.startsWith('file_') && reference > 0) {
        metrics[`file_loading.${story}.ratio_to_file`] = value / reference;
      }
    }
  }
  return metrics;
}

// samples[metric][targetId] = one value per round.
export function collectSamples(resultsDir, metadata) {
  const samples = {};
  for (const invocation of metadata.invocations) {
    const metrics = collectInvocation(path.join(resultsDir, invocation.dir), invocation.suite);
    for (const [metric, value] of Object.entries(metrics)) {
      samples[metric] ??= {};
      (samples[metric][invocation.target] ??= []).push(value);
    }
  }
  return samples;
}

const lowerIsBetter = (metric) => !metric.endsWith('.score');

export function compareTargets(samples, targets, { alpha, minChange }) {
  const ids = new Set(targets.map((target) => target.id));
  return COMPARISONS.filter((c) => ids.has(c.reference) && ids.has(c.candidate)).map((comparison) => ({
    ...comparison,
    metrics: Object.keys(samples)
      .sort()
      .filter((metric) => samples[metric][comparison.reference] && samples[metric][comparison.candidate])
      .map((metric) => ({
        metric,
        lowerIsBetter: lowerIsBetter(metric),
        ...compareSamples(samples[metric][comparison.reference], samples[metric][comparison.candidate], {
          lowerIsBetter: lowerIsBetter(metric),
          alpha,
          minChange
        })
      }))
  }));
}

const describeTarget = (target) =>
  target.kind === 'electron'
    ? `Electron ${target.electronVersion} (Chromium ${target.chromiumVersion})`
    : `Chrome for Testing ${target.chromiumVersion}`;

function formatValue(metric, value) {
  if (!Number.isFinite(value)) return 'n/a';
  if (metric.endsWith('.ratio_to_file')) return `${value.toFixed(2)}x`;
  if (metric.endsWith('.score')) return value.toFixed(2);
  return `${value.toFixed(1)} ms`;
}

const formatP = (p) => (Number.isFinite(p) ? (p < 0.001 ? '<0.001' : p.toFixed(3)) : 'n/a');
const formatChange = (change) =>
  Number.isFinite(change) ? `${change >= 0 ? '+' : ''}${(change * 100).toFixed(1)}%` : 'n/a';
const VERDICT_LABELS = { regression: ':red_circle: regression', improvement: ':green_circle: improvement' };

export function markdownSummary({ metadata, comparisons, failures, alpha, minChange }) {
  const targets = Object.fromEntries(metadata.targets.map((target) => [target.id, target]));
  const lines = [
    '## Electron benchmarks',
    '',
    `${metadata.rounds} interleaved rounds on ${metadata.cpuModel} (${metadata.cpuCount} CPUs, ${metadata.runnerName}${metadata.vmSize ? `, ${metadata.vmSize}` : ''}), crossbench ${metadata.crossbenchRevision.slice(0, 12)}.`,
    `Flagged when Welch's t-test and Mann-Whitney U both give p < ${alpha} and the medians differ by at least ${(minChange * 100).toFixed(0)}%. Thresholds are not calibrated yet.`,
    ''
  ];
  if (failures.length) {
    lines.push(
      `:warning: ${failures.length} crossbench runs failed: ${failures.map((f) => `${f.suite}/${f.target} (round ${f.round + 1})`).join(', ')}`,
      ''
    );
  }
  for (const comparison of comparisons) {
    lines.push(
      `### ${comparison.title}`,
      '',
      `Reference: ${describeTarget(targets[comparison.reference])}<br>Candidate: ${describeTarget(targets[comparison.candidate])}`,
      '',
      '| Metric | Reference | Candidate | Change | p (Welch) | p (MWU) | n | Verdict |',
      '| --- | --: | --: | --: | --: | --: | --: | --- |'
    );
    for (const m of comparison.metrics) {
      lines.push(
        `| ${m.metric} | ${formatValue(m.metric, m.referenceMedian)} | ${formatValue(m.metric, m.candidateMedian)} | ${formatChange(m.relativeChange)} | ${formatP(m.welchP)} | ${formatP(m.mannWhitneyP)} | ${m.referenceCount}/${m.candidateCount} | ${VERDICT_LABELS[m.verdict] ?? m.verdict} |`
      );
    }
    lines.push('');
  }
  return lines.join('\n');
}

// Datadog tags allow lowercase alphanumerics and _ - : . /
const tag = (key, value) =>
  `${key}:${String(value)
    .toLowerCase()
    .replace(/[^a-z0-9_.:/-]+/g, '_')
    .replace(/^_+|_+$/g, '')
    .slice(0, 150)}`;

export function datadogSeries({ metadata, samples, comparisons }) {
  const timestamp = Math.round(Date.parse(metadata.finishedAt ?? metadata.startedAt) / 1000);
  const targets = Object.fromEntries(metadata.targets.map((target) => [target.id, target]));
  const common = [tag('platform', metadata.platform), tag('arch', metadata.arch), tag('cpu_model', metadata.cpuModel)];
  const series = [];

  for (const [metric, byTarget] of Object.entries(samples)) {
    for (const [id, values] of Object.entries(byTarget)) {
      const target = targets[id];
      series.push({
        metric: `electron.perf.${metric}`,
        type: 3, // GAUGE
        points: [{ timestamp, value: median(values) }],
        tags: [
          ...common,
          tag('benchmark', metric.split('.')[0]),
          tag('target', id),
          tag('electron_version', target.electronVersion ?? 'none'),
          tag('chromium_version', target.chromiumVersion)
        ]
      });
    }
  }

  for (const comparison of comparisons) {
    const candidate = targets[comparison.candidate];
    const reference = targets[comparison.reference];
    for (const m of comparison.metrics) {
      if (!Number.isFinite(m.relativeChange)) continue;
      series.push({
        metric: `electron.perf.delta.${m.metric}`,
        type: 3, // GAUGE
        unit: 'percent',
        points: [{ timestamp, value: m.relativeChange * 100 }],
        tags: [
          ...common,
          tag('benchmark', m.metric.split('.')[0]),
          tag('comparison', comparison.name),
          tag('electron_version', candidate.electronVersion),
          tag('chromium_version', candidate.chromiumVersion),
          tag('reference_version', reference.electronVersion ?? reference.chromiumVersion),
          tag('verdict', m.verdict)
        ]
      });
    }
  }
  return series;
}

async function uploadSeriesToDatadog(series) {
  // Same endpoint and auth as script/build-stats.mjs.
  const response = await fetch('https://api.datadoghq.com/api/v2/series', {
    method: 'POST',
    headers: {
      'Content-Type': 'application/json',
      'DD-API-KEY': process.env.DD_API_KEY
    },
    body: JSON.stringify({ series })
  });
  if (!response.ok) throw new Error(`Datadog upload failed: ${response.status} ${await response.text()}`);
}

async function main() {
  const { values: args } = parseArgs({
    options: {
      results: { type: 'string' },
      'min-change': { type: 'string', default: '0.1' },
      alpha: { type: 'string', default: '0.05' },
      'dry-run': { type: 'boolean', default: false },
      'fail-on-regression': { type: 'boolean', default: false }
    }
  });
  if (!args.results) throw new Error('--results is required');
  const alpha = parseFloat(args.alpha);
  const minChange = parseFloat(args['min-change']);
  const resultsDir = path.resolve(args.results);
  const metadata = readJson(path.join(resultsDir, 'metadata.json'));

  const samples = collectSamples(resultsDir, metadata);
  const comparisons = compareTargets(samples, metadata.targets, { alpha, minChange });
  const failures = metadata.invocations.filter((invocation) => invocation.exitCode !== 0);
  fs.writeFileSync(
    path.join(resultsDir, 'comparison.json'),
    JSON.stringify({ alpha, minChange, metadata, samples, comparisons, failures }, null, 2)
  );

  const summary = markdownSummary({ metadata, comparisons, failures, alpha, minChange });
  fs.writeFileSync(path.join(resultsDir, 'summary.md'), summary);
  if (process.env.GITHUB_STEP_SUMMARY) {
    fs.appendFileSync(process.env.GITHUB_STEP_SUMMARY, `${summary}\n`);
  } else {
    console.log(summary);
  }

  const series = datadogSeries({ metadata, samples, comparisons });
  if (args['dry-run']) {
    console.log(`[compare] dry run: would upload ${series.length} series to Datadog`);
    console.log(JSON.stringify({ series }, null, 2));
  } else if (process.env.DD_API_KEY) {
    await uploadSeriesToDatadog(series);
    console.log(`[compare] uploaded ${series.length} series to Datadog`);
  } else {
    console.log('[compare] DD_API_KEY is not set; skipping the Datadog upload');
  }

  const regressions = comparisons.flatMap((c) =>
    c.metrics.filter((m) => m.verdict === 'regression').map((m) => `${c.name} ${m.metric}`)
  );
  if (regressions.length) {
    const prefix = process.env.GITHUB_ACTIONS ? '::warning title=Benchmark regression::' : '';
    console.log(`${prefix}${regressions.length} regressions: ${regressions.join(', ')}`);
    if (args['fail-on-regression']) process.exit(1);
  }
}

if (process.argv[1] && fs.realpathSync(process.argv[1]) === fileURLToPath(import.meta.url)) {
  main().catch((error) => {
    console.error(`ERROR: ${error.message}`);
    process.exit(1);
  });
}
