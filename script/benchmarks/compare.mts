// Compares the targets of one interleaved benchmark run (see run.mts).
//
// For every metric it takes one sample per round and target (the median of
// that round's repetitions), then compares the candidate against the baseline
// with Welch's t-test and the Mann-Whitney U test. A change is flagged only when both tests give p < alpha AND the
// medians differ by at least --min-change.
//
// Writes <results>/comparison.json and a Markdown table (to
// $GITHUB_STEP_SUMMARY when set), and uploads to Datadog when DD_API_KEY is
// set and --dry-run is not passed:
//   electron.perf.<metric>        median per target
//   electron.perf.delta.<metric>  relative change in percent per comparison
//
// Usage:
//   node script/benchmarks/compare.mts --results <dir> [--min-change 0.1]
//     [--alpha 0.05] [--dry-run] [--fail-on-regression]
import * as fs from 'node:fs';
import * as path from 'node:path';
import { fileURLToPath } from 'node:url';
import { parseArgs } from 'node:util';

import { compareSamples, median, type SampleComparison } from './lib/stats.mts';

import type { Invocation, RunMetadata, TargetInfo } from './lib/types.mts';

// samples[metric][targetId] = one value per round.
export type Samples = Record<string, Record<string, number[]>>;

export interface Rejected {
  round: number;
  suite: string;
  target: string;
  reason: string;
}

interface ComparisonDefinition {
  name: string;
  reference: string;
  candidate: string;
  title: string;
}

export interface MetricComparison extends SampleComparison {
  metric: string;
  lowerIsBetter: boolean;
}

export interface Comparison extends ComparisonDefinition {
  metrics: MetricComparison[];
}

// Suites run through crossbench's `loading` benchmark with generated page
// configs (see lib/configs.mts); each reports one value per repetition.
const PAGE_SUITES = ['loading', 'trace', 'file_loading'];

// Measures read from crossbench's performance.entries probe, which merges
// every repetition of a story into <story>/performance.entries.json.
const PERFORMANCE_ENTRIES: Record<string, Record<string, string>> = {
  loading: { fcp: 'paint/first-contentful-paint/startTime', load_event_end: 'navigation/loadEventEnd' },
  file_loading: { files_loaded_ms: 'mark/files-loaded/startTime' }
};

const PRESS_RESULTS: Record<string, { file: string; keys: string[] }> = {
  speedometer: { file: 'speedometer_3.1.json', keys: ['Score'] },
  jetstream: { file: 'jetstream_2.2.json', keys: ['Total/Score'] }
};

const COMPARISONS: ComparisonDefinition[] = [
  { name: 'vs_baseline', reference: 'baseline', candidate: 'candidate', title: 'Candidate vs baseline' }
];

const readJson = (file: string): any => JSON.parse(fs.readFileSync(file, 'utf8'));

// Values of `data[key].values`, as written by crossbench's metrics merger.
const valuesOf = (entry: any): number[] => (Array.isArray(entry?.values) ? entry.values.filter(Number.isFinite) : []);

// Returns { metrics: { metric: number }, incomplete: [description] } for one
// crossbench invocation. A measure with fewer (or more) values than the
// expected repetitions is dropped, so a run where some repetitions failed or
// timed out can't bias the round's median.
function collectInvocation(
  resultDir: string,
  suite: string,
  expectedCount: number | undefined
): { metrics: Record<string, number>; incomplete: string[] } {
  const metrics: Record<string, number> = {};
  const incomplete: string[] = [];
  const complete = (name: string, values: number[]) => {
    if (values.length === 0) return false;
    if (expectedCount === undefined || values.length === expectedCount) return true;
    incomplete.push(`${name} (${values.length}/${expectedCount} values)`);
    return false;
  };
  if (!fs.existsSync(resultDir)) return { metrics, incomplete };

  if (PRESS_RESULTS[suite]) {
    const file = path.join(resultDir, PRESS_RESULTS[suite].file);
    if (!fs.existsSync(file)) return { metrics, incomplete };
    for (const browser of Object.values<any>(readJson(file))) {
      const key = PRESS_RESULTS[suite].keys.find((k) => valuesOf(browser.data?.[k]).length > 0);
      const values = key ? valuesOf(browser.data[key]) : [];
      if (complete(`${suite}.score`, values)) metrics[`${suite}.score`] = median(values);
    }
    return { metrics, incomplete };
  }

  // Each invocation runs one browser, so there is one <browser>/stories dir.
  for (const browserDir of fs.readdirSync(resultDir)) {
    const storiesDir = path.join(resultDir, browserDir, 'stories');
    if (!fs.existsSync(storiesDir) || !fs.statSync(storiesDir).isDirectory()) continue;
    for (const story of fs.readdirSync(storiesDir)) {
      const storyDir = path.join(storiesDir, story);
      if (!fs.statSync(storyDir).isDirectory()) continue;
      for (const [measure, values] of Object.entries(storyMeasures(suite, story, storyDir))) {
        const metric = `${suite}.${story}.${measure}`;
        if (values.length === 0) incomplete.push(`${metric} (no values)`);
        else if (complete(metric, values)) metrics[metric] = median(values);
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
  return { metrics, incomplete };
}

// The run directories of one story, one per repetition, in order:
// <story>/<repetition>/<cache temperature>/.
function runDirs(storyDir: string): string[] {
  const dirs: string[] = [];
  const repetitions = fs
    .readdirSync(storyDir)
    .filter((entry) => /^\d+$/.test(entry))
    .sort((a, b) => Number(a) - Number(b));
  for (const repetition of repetitions) {
    const repetitionDir = path.join(storyDir, repetition);
    for (const temperature of fs.readdirSync(repetitionDir).sort()) {
      const runDir = path.join(repetitionDir, temperature);
      if (fs.statSync(runDir).isDirectory()) dirs.push(runDir);
    }
  }
  return dirs;
}

const readJsonIfExists = (file: string): any => (fs.existsSync(file) ? readJson(file) : undefined);
const isNumber = (value: unknown): value is number => typeof value === 'number' && Number.isFinite(value);

// The nearest-rank percentile of a sorted list.
const percentile = (sorted: number[], p: number) => sorted[Math.min(sorted.length - 1, Math.floor(sorted.length * p))];

// Process type of one meminfo entry: the --type switch, 'browser' for the
// main process, or undefined for processes that are not part of the browser
// (the shell crossbench's meminfo script runs in matches its own pgrep).
function processType(cmdline: string): string | undefined {
  const program = path.basename(cmdline.trim().split(/\s+/)[0] ?? '');
  if (/^(ba|da)?sh$/.test(program)) return undefined;
  const type = /--type=(\S+)/.exec(cmdline)?.[1];
  if (type) return type;
  return program.includes('crashpad') ? 'crashpad' : 'browser';
}

// PSS in MiB per process type from one meminfo dump (smaps_rollup values
// are in kB).
export function memoryMeasures(dump: any): Record<string, number> {
  const kb: Record<string, number> = { total: 0, browser: 0, renderer: 0, gpu: 0 };
  for (const process of dump?.processes ?? []) {
    const type = processType(String(process.name ?? ''));
    if (!type || !isNumber(process.pss_total)) continue;
    kb.total += process.pss_total;
    if (type === 'browser') kb.browser += process.pss_total;
    else if (type === 'renderer') kb.renderer += process.pss_total;
    else if (type === 'gpu-process') kb.gpu += process.pss_total;
  }
  return Object.fromEntries(Object.entries(kb).map(([type, value]) => [`${type}_pss_mib`, value / 1024]));
}

const MEMORY_MEASURES = ['total_pss_mib', 'browser_pss_mib', 'renderer_pss_mib', 'gpu_pss_mib'];

// Measures each story of a page suite reports. A story that reports none of
// them (for example after a Chromium roll renamed a trace event) is listed as
// incomplete instead of disappearing from the results.
function expectedMeasures(suite: string, story: string): string[] {
  if (suite === 'trace') return story === 'resize' ? ['resize_median_ms', 'resize_p90_ms'] : ['fcp', 'lcp'];
  const measures = Object.keys(PERFORMANCE_ENTRIES[suite] ?? {});
  return suite === 'loading' && story === 'first_load' ? [...measures, ...MEMORY_MEASURES] : measures;
}

// Values per measure (one per repetition) for one story of a page suite.
function storyMeasures(suite: string, story: string, storyDir: string): Record<string, number[]> {
  const measures: Record<string, number[]> = Object.fromEntries(
    expectedMeasures(suite, story).map((measure) => [measure, []])
  );
  const add = (measure: string, value: unknown) => {
    if (isNumber(value) && measures[measure]) measures[measure].push(value);
  };

  const entries = PERFORMANCE_ENTRIES[suite];
  if (entries) {
    const data = readJsonIfExists(path.join(storyDir, 'performance.entries.json'));
    for (const [measure, key] of Object.entries(entries)) {
      for (const value of valuesOf(data?.[key])) add(measure, value);
    }
  }

  for (const runDir of runDirs(storyDir)) {
    if (suite === 'loading') {
      // One dump per run, from the `meminfo` action at the end of the story.
      const meminfoDir = path.join(runDir, 'meminfo');
      const dumps = fs.existsSync(meminfoDir) ? fs.readdirSync(meminfoDir).filter((f) => f.endsWith('.json')) : [];
      if (dumps.length === 1) {
        for (const [measure, value] of Object.entries(memoryMeasures(readJson(path.join(meminfoDir, dumps[0]))))) {
          add(measure, value);
        }
      }
    } else if (suite === 'trace') {
      const tpDir = path.join(runDir, 'trace_processor');
      if (story === 'resize') {
        const rows: any[] = readJsonIfExists(path.join(tpDir, 'resize.json')) ?? [];
        const latencies = rows
          .map((row) => row.resize_ms)
          .filter(isNumber)
          .sort((a, b) => a - b);
        if (latencies.length > 0) {
          add('resize_median_ms', median(latencies));
          add('resize_p90_ms', percentile(latencies, 0.9));
        }
      } else {
        const [row] = readJsonIfExists(path.join(tpDir, 'page_load.json')) ?? [];
        add('fcp', row?.fcp);
        add('lcp', row?.lcp);
      }
    }
  }
  return measures;
}

// Repetitions one invocation should report per measure. run.mts records it;
// older metadata only has the loading suites' --repeat.
function expectedCountOf(invocation: Invocation, metadata: RunMetadata): number | undefined {
  if (Number.isInteger(invocation.repetitions)) return invocation.repetitions;
  if (PAGE_SUITES.includes(invocation.suite)) return metadata.repeat;
  return undefined;
}

// samples[metric][targetId] = one value per round. Only invocations that
// exited with 0 contribute, and only measures with every repetition present.
// rejected lists what was left out and why.
export function collectSamples(resultsDir: string, metadata: RunMetadata): { samples: Samples; rejected: Rejected[] } {
  const samples: Samples = {};
  const rejected: Rejected[] = [];
  for (const invocation of metadata.invocations) {
    const { round, suite, target } = invocation;
    if (invocation.exitCode !== 0) {
      rejected.push({ round, suite, target, reason: `crossbench exited with ${invocation.exitCode}` });
      continue;
    }
    const { metrics, incomplete } = collectInvocation(
      path.join(resultsDir, invocation.dir),
      suite,
      expectedCountOf(invocation, metadata)
    );
    if (incomplete.length) rejected.push({ round, suite, target, reason: `incomplete: ${incomplete.join(', ')}` });
    for (const [metric, value] of Object.entries(metrics)) {
      samples[metric] ??= {};
      (samples[metric][target] ??= []).push(value);
    }
  }
  return { samples, rejected };
}

const lowerIsBetter = (metric: string) => !metric.endsWith('.score');

// Reasons the results can't be trusted as a whole: no samples at all, or a
// comparison without a single comparable metric.
export function findProblems(samples: Samples, comparisons: Comparison[], targets: TargetInfo[]): string[] {
  const problems: string[] = [];
  if (Object.keys(samples).length === 0) problems.push('no usable benchmark results');
  const ids = new Set(targets.map((target) => target.id));
  for (const { name, reference, candidate } of COMPARISONS) {
    const comparison = comparisons.find((c) => c.name === name);
    if (!comparison) {
      problems.push(`${name}: missing target ${ids.has(reference) ? candidate : reference}`);
    } else if (comparison.metrics.length === 0) {
      problems.push(`${name}: no metric has results for both ${reference} and ${candidate}`);
    }
  }
  return problems;
}

export function compareTargets(
  samples: Samples,
  targets: TargetInfo[],
  { alpha, minChange }: { alpha: number; minChange: number }
): Comparison[] {
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

const describeTarget = (target: TargetInfo) =>
  `Electron ${target.electronVersion} (Chromium ${target.chromiumVersion})`;

function formatValue(metric: string, value: number) {
  if (!Number.isFinite(value)) return 'n/a';
  if (metric.endsWith('.ratio_to_file')) return `${value.toFixed(2)}x`;
  if (metric.endsWith('.score')) return value.toFixed(2);
  if (metric.endsWith('_mib')) return `${value.toFixed(1)} MiB`;
  return `${value.toFixed(1)} ms`;
}

const formatP = (p: number) => (Number.isFinite(p) ? (p < 0.001 ? '<0.001' : p.toFixed(3)) : 'n/a');
const formatChange = (change: number) =>
  Number.isFinite(change) ? `${change >= 0 ? '+' : ''}${(change * 100).toFixed(1)}%` : 'n/a';
const VERDICT_LABELS: Record<string, string> = {
  regression: ':red_circle: regression',
  improvement: ':green_circle: improvement'
};

interface SummaryOptions {
  metadata: RunMetadata;
  comparisons: Comparison[];
  rejected: Rejected[];
  problems?: string[];
  alpha: number;
  minChange: number;
}

export function markdownSummary({ metadata, comparisons, rejected, problems = [], alpha, minChange }: SummaryOptions) {
  const targets = Object.fromEntries(metadata.targets.map((target) => [target.id, target]));
  const lines = [
    '## Electron benchmarks',
    '',
    `${metadata.rounds} interleaved rounds on ${metadata.cpuModel} (${metadata.cpuCount} CPUs, ${metadata.runnerName}${metadata.vmSize ? `, ${metadata.vmSize}` : ''}), crossbench ${metadata.crossbenchRevision.slice(0, 12)}.`,
    `Flagged when Welch's t-test and Mann-Whitney U both give p < ${alpha} and the medians differ by at least ${(minChange * 100).toFixed(0)}%. Thresholds are not calibrated yet.`,
    ''
  ];
  for (const problem of problems) lines.push(`:x: ${problem}`, '');
  if (rejected.length) {
    lines.push(
      `:warning: Left out of the comparison: ${rejected.map((r) => `${r.suite}/${r.target} round ${r.round + 1} (${r.reason})`).join('; ')}`,
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
const tag = (key: string, value: unknown) =>
  `${key}:${String(value)
    .toLowerCase()
    .replace(/[^a-z0-9_.:/-]+/g, '_')
    .replace(/^_+|_+$/g, '')
    .slice(0, 150)}`;

// Points are stamped at upload time, as script/build-stats.mjs does: the v2
// series intake rejects points more than an hour old, and a run cut off by a
// timeout or cancellation can be hours past its start.
interface Series {
  metric: string;
  type: number;
  unit?: string;
  points: { timestamp: number; value: number }[];
  tags: string[];
}

export function datadogSeries(
  { metadata, samples, comparisons }: { metadata: RunMetadata; samples: Samples; comparisons: Comparison[] },
  now = Date.now()
): Series[] {
  const timestamp = Math.round(now / 1000);
  const targets = Object.fromEntries(metadata.targets.map((target) => [target.id, target]));
  const common = [tag('platform', metadata.platform), tag('arch', metadata.arch), tag('cpu_model', metadata.cpuModel)];
  const series: Series[] = [];

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
          tag('electron_version', target.electronVersion),
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
          tag('reference_version', reference.electronVersion),
          tag('verdict', m.verdict)
        ]
      });
    }
  }
  return series;
}

async function uploadSeriesToDatadog(series: Series[]) {
  // Same endpoint and auth as script/build-stats.mjs.
  const response = await fetch('https://api.datadoghq.com/api/v2/series', {
    method: 'POST',
    headers: {
      'Content-Type': 'application/json',
      'DD-API-KEY': process.env.DD_API_KEY ?? ''
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
  const metadata: RunMetadata = readJson(path.join(resultsDir, 'metadata.json'));

  const { samples, rejected } = collectSamples(resultsDir, metadata);
  for (const r of rejected) {
    console.warn(`[compare] leaving out ${r.suite}/${r.target} round ${r.round + 1}: ${r.reason}`);
  }
  const comparisons = compareTargets(samples, metadata.targets, { alpha, minChange });
  const problems = findProblems(samples, comparisons, metadata.targets);
  fs.writeFileSync(
    path.join(resultsDir, 'comparison.json'),
    JSON.stringify({ alpha, minChange, metadata, samples, comparisons, rejected, problems }, null, 2)
  );

  const summary = markdownSummary({ metadata, comparisons, rejected, problems, alpha, minChange });
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
    if (args['fail-on-regression']) process.exitCode = 1;
  }

  if (problems.length) {
    const prefix = process.env.GITHUB_ACTIONS ? '::error title=Benchmark results::' : 'ERROR: ';
    for (const problem of problems) console.error(`${prefix}${problem}`);
    process.exitCode = 1;
  }
}

if (process.argv[1] && fs.realpathSync(process.argv[1]) === fileURLToPath(import.meta.url)) {
  main().catch((error: unknown) => {
    console.error(`ERROR: ${error instanceof Error ? error.message : String(error)}`);
    process.exit(1);
  });
}
