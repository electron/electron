// Run with: node --test script/benchmarks/test/*.test.mts
import assert from 'node:assert/strict';
import { spawnSync } from 'node:child_process';
import * as fs from 'node:fs';
import * as os from 'node:os';
import * as path from 'node:path';
import { after, describe, it } from 'node:test';
import { fileURLToPath } from 'node:url';

import {
  collectSamples,
  compareTargets,
  datadogSeries,
  findProblems,
  historySeries,
  memoryMeasures,
  type Samples
} from '../compare.mts';

import type { RunMetadata, TargetInfo } from '../lib/types.mts';

const COMPARE = fileURLToPath(new URL('../compare.mts', import.meta.url));
const tmp = fs.mkdtempSync(path.join(os.tmpdir(), 'benchmarks-compare-test-'));
after(() => fs.rmSync(tmp, { recursive: true, force: true }));

const TARGETS: TargetInfo[] = [
  { id: 'candidate', electronVersion: '47.0.0', chromiumVersion: '158.0.1.0' },
  { id: 'baseline', electronVersion: '46.0.0', chromiumVersion: '157.0.1.0' }
];

// Writes a results dir laid out like run.mts and crossbench leave it. Each
// invocation gets a first_load story whose performance.entries FCP has
// `values` (default: one value per repetition), loadEventEnd 10 ms later and
// a meminfo dump per value, unless `values` is null.
interface TestInvocation {
  round: number;
  target: string;
  exitCode?: number;
  values?: number[] | null;
}

function writeResults(
  name: string,
  invocations: TestInvocation[],
  { repeat = 3, targets = TARGETS }: { repeat?: number; targets?: TargetInfo[] } = {}
) {
  const dir = path.join(tmp, name);
  fs.mkdirSync(dir, { recursive: true });
  const metadata: RunMetadata = {
    startedAt: '2026-10-29T00:00:00.000Z',
    platform: 'linux',
    arch: 'x64',
    cpuModel: 'test cpu',
    cpuCount: 4,
    runnerName: 'test',
    crossbenchRevision: '0123456789abcdef',
    rounds: 1 + Math.max(...invocations.map((i) => i.round)),
    repeat,
    targets,
    invocations: []
  };
  for (const { round, target, exitCode = 0, values } of invocations) {
    const relativeDir = path.join('rounds', String(round), 'loading', target);
    metadata.invocations.push({ round, suite: 'loading', target, exitCode, repetitions: repeat, dir: relativeDir });
    if (values === null) continue;
    const storyDir = path.join(dir, relativeDir, `chromium_${target}`, 'stories', 'first_load');
    fs.mkdirSync(storyDir, { recursive: true });
    const fcp = values ?? Array.from({ length: repeat }, (_, i) => 100 + round + i);
    fs.writeFileSync(
      path.join(storyDir, 'performance.entries.json'),
      JSON.stringify({
        'paint/first-contentful-paint/startTime': { values: fcp },
        'navigation/loadEventEnd': { values: fcp.map((value) => value + 10) }
      })
    );
    fcp.forEach((_, repetition) => {
      const meminfoDir = path.join(storyDir, String(repetition), '0_default', 'meminfo');
      fs.mkdirSync(meminfoDir, { recursive: true });
      fs.writeFileSync(
        path.join(meminfoDir, 'loaded.json'),
        JSON.stringify({ processes: [{ pid: 1, name: '/opt/electron/electron', pss_total: 1024 }] })
      );
    });
  }
  fs.writeFileSync(path.join(dir, 'metadata.json'), JSON.stringify(metadata));
  return { dir, metadata };
}

const everyTarget = (round: number, overrides: Record<string, Partial<TestInvocation>> = {}): TestInvocation[] =>
  TARGETS.map(({ id }) => ({ round, target: id, ...overrides[id] }));

const runCompare = (dir: string) =>
  spawnSync(process.execPath, [COMPARE, '--results', dir, '--dry-run'], {
    encoding: 'utf8',
    env: { ...process.env, GITHUB_STEP_SUMMARY: '', GITHUB_ACTIONS: '' }
  });

describe('collectSamples', () => {
  it('takes one sample per round and target from successful invocations', () => {
    const { dir, metadata } = writeResults('ok', [...everyTarget(0), ...everyTarget(1)]);
    const { samples, rejected } = collectSamples(dir, metadata);
    assert.deepEqual(samples['loading.first_load.fcp'], {
      candidate: [101, 102],
      baseline: [101, 102]
    });
    assert.deepEqual(rejected, []);
  });

  it("leaves out a failed invocation's results", () => {
    const { dir, metadata } = writeResults('failed', [
      ...everyTarget(0),
      ...everyTarget(1, { candidate: { exitCode: 1, values: [500, 500, 500] } })
    ]);
    const { samples, rejected } = collectSamples(dir, metadata);
    assert.deepEqual(samples['loading.first_load.fcp'].candidate, [101]);
    assert.deepEqual(rejected, [
      { round: 1, suite: 'loading', target: 'candidate', reason: 'crossbench exited with 1' }
    ]);
  });

  it('leaves out measures with missing repetitions', () => {
    const { dir, metadata } = writeResults('incomplete', [
      ...everyTarget(0),
      ...everyTarget(1, { baseline: { values: [90, 91] } })
    ]);
    const { samples, rejected } = collectSamples(dir, metadata);
    assert.deepEqual(samples['loading.first_load.fcp'].baseline, [101]);
    assert.equal(rejected.length, 1);
    assert.match(rejected[0].reason, /loading\.first_load\.fcp \(2\/3 values\)/);
  });
});

// Writes one trace-suite invocation: per repetition, the trace_processor
// query results crossbench leaves in <story>/<rep>/0_default/trace_processor/.
function writeTraceResults(name: string, runs: { pageLoad: object[]; resize?: object[] }[]) {
  const dir = path.join(tmp, name);
  const relativeDir = path.join('rounds', '0', 'trace', 'candidate');
  const storiesDir = path.join(dir, relativeDir, 'chromium_candidate', 'stories');
  runs.forEach(({ pageLoad, resize }, repetition) => {
    const firstLoad = path.join(storiesDir, 'first_load', String(repetition), '0_default', 'trace_processor');
    fs.mkdirSync(firstLoad, { recursive: true });
    fs.writeFileSync(path.join(firstLoad, 'page_load.json'), JSON.stringify(pageLoad));
    if (resize) {
      const resizeDir = path.join(storiesDir, 'resize', String(repetition), '0_default', 'trace_processor');
      fs.mkdirSync(resizeDir, { recursive: true });
      fs.writeFileSync(path.join(resizeDir, 'resize.json'), JSON.stringify(resize));
    }
  });
  const metadata: RunMetadata = {
    startedAt: '2026-10-29T00:00:00.000Z',
    platform: 'linux',
    arch: 'x64',
    cpuModel: 'test cpu',
    cpuCount: 4,
    runnerName: 'test',
    crossbenchRevision: '0123456789abcdef',
    rounds: 1,
    repeat: runs.length,
    targets: TARGETS,
    invocations: [{ round: 0, suite: 'trace', target: 'candidate', exitCode: 0, repetitions: 2, dir: relativeDir }]
  };
  return { dir, metadata };
}

describe('trace suite', () => {
  const resize = (values: (number | null)[]) => values.map((resize_ms, i) => ({ resize_ts: i, resize_ms }));

  it('reads FCP, LCP and resize latency from the trace_processor results', () => {
    const { dir, metadata } = writeTraceResults('trace-ok', [
      { pageLoad: [{ fcp: 80, lcp: 90 }], resize: resize([30, 10, 20, 40, null]) },
      { pageLoad: [{ fcp: 100, lcp: 110 }], resize: resize([50, 60, 70, 80, 90]) }
    ]);
    const { samples, rejected } = collectSamples(dir, metadata);
    assert.deepEqual(rejected, []);
    assert.deepEqual(samples['trace.first_load.fcp'].candidate, [90]);
    assert.deepEqual(samples['trace.first_load.lcp'].candidate, [100]);
    // Per repetition: median 25 and 70, p90 40 and 90.
    assert.deepEqual(samples['trace.resize.resize_median_ms'].candidate, [47.5]);
    assert.deepEqual(samples['trace.resize.resize_p90_ms'].candidate, [65]);
  });

  it('leaves out a measure when a repetition has no trace result', () => {
    const { dir, metadata } = writeTraceResults('trace-missing', [
      { pageLoad: [{ fcp: 80, lcp: 90 }] },
      { pageLoad: [] }
    ]);
    const { samples, rejected } = collectSamples(dir, metadata);
    assert.equal(samples['trace.first_load.fcp'], undefined);
    assert.match(rejected[0].reason, /trace\.first_load\.fcp \(1\/2 values\)/);
  });

  it('reports a story whose trace has none of the expected events', () => {
    const { dir, metadata } = writeTraceResults('trace-empty', [
      { pageLoad: [{ fcp: 80, lcp: 90 }], resize: resize([]) },
      { pageLoad: [{ fcp: 100, lcp: 110 }], resize: resize([]) }
    ]);
    const { samples, rejected } = collectSamples(dir, metadata);
    assert.deepEqual(samples['trace.first_load.fcp'].candidate, [90]);
    assert.equal(samples['trace.resize.resize_median_ms'], undefined);
    assert.match(rejected[0].reason, /trace\.resize\.resize_median_ms \(no values\)/);
  });
});

describe('memoryMeasures', () => {
  it('sums PSS per process type and ignores the meminfo shell', () => {
    const app = '/opt/electron/electron';
    const measures = memoryMeasures({
      processes: [
        { pid: 1, name: `${app} --app=/harness`, pss_total: 2048 },
        { pid: 2, name: `${app} --type=renderer --enable-blink`, pss_total: 1024 },
        { pid: 3, name: `${app} --type=renderer`, pss_total: 1024 },
        { pid: 4, name: `${app} --type=gpu-process`, pss_total: 512 },
        { pid: 5, name: `${app} --type=utility --utility-sub-type=network.mojom.NetworkService`, pss_total: 512 },
        { pid: 6, name: `bash /tmp/meminfo.sh ${app}`, pss_total: 4096 }
      ]
    });
    assert.deepEqual(measures, {
      total_pss_mib: 5,
      browser_pss_mib: 2,
      renderer_pss_mib: 2,
      gpu_pss_mib: 0.5
    });
  });
});

describe('findProblems', () => {
  const problemsFor = (name: string, invocations: TestInvocation[]) => {
    const { dir, metadata } = writeResults(name, invocations);
    const { samples } = collectSamples(dir, metadata);
    const comparisons = compareTargets(samples, metadata.targets, { alpha: 0.05, minChange: 0.1 });
    return findProblems(samples, comparisons, metadata.targets);
  };

  it('accepts results with every comparison', () => {
    assert.deepEqual(problemsFor('complete', everyTarget(0)), []);
  });

  it('reports a comparison without results', () => {
    const problems = problemsFor('no-baseline', everyTarget(0, { baseline: { exitCode: 1, values: null } }));
    assert.deepEqual(problems, ['vs_baseline: no metric has results for both baseline and candidate']);
  });
});

describe('datadogSeries', () => {
  it('stamps points at upload time for a run that never finished', () => {
    // run.mts was killed, so metadata.json has startedAt (hours ago) but no
    // finishedAt.
    const { dir, metadata } = writeResults('unfinished', everyTarget(0));
    assert.equal(metadata.finishedAt, undefined);
    const { samples } = collectSamples(dir, metadata);
    const comparisons = compareTargets(samples, metadata.targets, { alpha: 0.05, minChange: 0.1 });
    const before = Math.floor(Date.now() / 1000);
    const series = datadogSeries({ metadata, samples, comparisons });
    const after = Math.ceil(Date.now() / 1000);
    assert.ok(series.length > 0);
    for (const { points } of series) {
      for (const { timestamp } of points) {
        assert.ok(timestamp >= before && timestamp <= after, `${timestamp} is not between ${before} and ${after}`);
      }
    }
  });
});

// Five rounds per target: FCP 20% slower and the Speedometer score 20%
// higher on the candidate (a regression and an improvement), load time
// unchanged.
const SAMPLES: Samples = {
  'loading.first_load.fcp': { baseline: [100, 101, 102, 103, 104], candidate: [120, 121, 122, 123, 124] },
  'loading.first_load.load_event_end': { baseline: [50, 52, 51, 53, 50], candidate: [51, 50, 53, 52, 51] },
  'speedometer.score': { baseline: [20, 20.2, 20.1, 20.3, 20.4], candidate: [24, 24.2, 24.1, 24.3, 24.4] }
};

const METADATA: RunMetadata = {
  startedAt: '2026-10-29T00:00:00.000Z',
  platform: 'linux',
  arch: 'x64',
  cpuModel: 'Test CPU @ 2.10GHz',
  cpuCount: 4,
  runnerName: 'test',
  crossbenchRevision: '0123456789abcdef',
  rounds: 5,
  repeat: 3,
  targets: TARGETS,
  invocations: []
};

const valueOf = (series: ReturnType<typeof datadogSeries>, metric: string, metricTag: string) => {
  const matching = series.filter((s) => s.metric === metric && s.tags.includes(metricTag));
  assert.equal(matching.length, 1, `expected one ${metric} series tagged ${metricTag}`);
  return matching[0];
};

describe('regression and improvement gauges', () => {
  const comparisons = compareTargets(SAMPLES, TARGETS, { alpha: 0.05, minChange: 0.1 });
  const series = datadogSeries({ metadata: METADATA, samples: SAMPLES, comparisons });

  it('reports 1 for the flagged verdict and 0 otherwise, for every metric', () => {
    const expected: Record<string, [number, number]> = {
      'loading.first_load.fcp': [1, 0],
      'loading.first_load.load_event_end': [0, 0],
      'speedometer.score': [0, 1]
    };
    for (const [metric, [regression, improvement]] of Object.entries(expected)) {
      assert.equal(valueOf(series, 'electron.perf.regression', `metric:${metric}`).points[0].value, regression);
      assert.equal(valueOf(series, 'electron.perf.improvement', `metric:${metric}`).points[0].value, improvement);
    }
  });

  it('follows the comparison verdict', () => {
    for (const m of comparisons[0].metrics) {
      const regression = valueOf(series, 'electron.perf.regression', `metric:${m.metric}`).points[0].value;
      assert.equal(regression === 1, m.verdict === 'regression', m.metric);
    }
  });

  it('reports nothing flagged when the change is below --min-change', () => {
    const lenient = compareTargets(SAMPLES, TARGETS, { alpha: 0.05, minChange: 0.5 });
    const flags = datadogSeries({ metadata: METADATA, samples: SAMPLES, comparisons: lenient }).filter((s) =>
      ['electron.perf.regression', 'electron.perf.improvement'].includes(s.metric)
    );
    assert.equal(flags.length, 6);
    assert.ok(flags.every((s) => s.points[0].value === 0));
  });

  it('is tagged with low-cardinality tags only', () => {
    const { tags } = valueOf(series, 'electron.perf.regression', 'metric:loading.first_load.fcp');
    assert.deepEqual(tags, [
      'platform:linux',
      'arch:x64',
      'metric:loading.first_load.fcp',
      'benchmark:loading',
      'comparison:vs_baseline',
      'electron_version:47.0.0',
      'baseline_version:46.0.0'
    ]);
  });

  it('leaves the existing series unchanged', () => {
    const delta = valueOf(series, 'electron.perf.delta.loading.first_load.fcp', 'comparison:vs_baseline');
    assert.ok(delta.tags.includes('verdict:regression'));
    assert.equal(series.filter((s) => s.metric === 'electron.perf.loading.first_load.fcp').length, 2);
  });
});

describe('historySeries', () => {
  const targets: TargetInfo[] = [
    { id: 'candidate', electronVersion: '30.5.1', chromiumVersion: '124.0.6367.243' },
    { id: 'baseline', electronVersion: '45.2.3', chromiumVersion: '156.0.8078.80' }
  ];
  const metadata = { ...METADATA, targets };
  const samples: Samples = {
    ...SAMPLES,
    'loading.first_load.gpu_pss_mib': { baseline: [90, 90, 90, 90, 90], candidate: [0, 0, 0, 0, 0] }
  };
  const comparisons = compareTargets(samples, targets, { alpha: 0.05, minChange: 0.1 });
  const series = historySeries({ metadata, comparisons, historyRunId: '123' });

  it('sends the candidate median and its ratio to the anchor', () => {
    const raw = valueOf(series, 'electron.perf.history.loading.first_load.fcp', 'electron_major:30');
    assert.equal(raw.points[0].value, 122);
    const ratio = valueOf(series, 'electron.perf.history.ratio.loading.first_load.fcp', 'electron_major:30');
    assert.equal(ratio.points[0].value, 122 / 102);
    const score = valueOf(series, 'electron.perf.history.ratio.speedometer.score', 'electron_major:30');
    assert.equal(score.points[0].value, 24.2 / 20.2);
  });

  it('tags each point with the versions and the run', () => {
    const { tags } = valueOf(series, 'electron.perf.history.ratio.loading.first_load.fcp', 'electron_major:30');
    assert.deepEqual(tags, [
      'platform:linux',
      'arch:x64',
      'cpu_model:test_cpu_2.10ghz',
      'electron_major:30',
      'electron_version:30.5.1',
      'chromium_version:124.0.6367.243',
      'anchor_version:45.2.3',
      'history_run_id:123',
      'benchmark:loading'
    ]);
  });

  it('leaves out values that are not positive', () => {
    assert.ok(!series.some((s) => s.metric.endsWith('gpu_pss_mib')));
  });

  it('sends no regression gauges or nightly series', () => {
    assert.ok(series.length > 0);
    assert.ok(series.every((s) => s.metric.startsWith('electron.perf.history.')));
  });
});

describe('compare.mts', () => {
  it('exits with 0 for complete results', () => {
    const { dir } = writeResults('cli-ok', [...everyTarget(0), ...everyTarget(1)]);
    const result = runCompare(dir);
    assert.equal(result.status, 0, result.stderr);
  });

  it('exits non-zero when every invocation failed', () => {
    const { dir } = writeResults('cli-all-failed', [
      ...everyTarget(0, Object.fromEntries(TARGETS.map(({ id }) => [id, { exitCode: 1, values: null }]))),
      ...everyTarget(1, Object.fromEntries(TARGETS.map(({ id }) => [id, { exitCode: 1, values: null }])))
    ]);
    const result = runCompare(dir);
    assert.equal(result.status, 1);
    assert.match(result.stderr, /no usable benchmark results/);
    // The diagnostics are still written.
    assert.ok(fs.existsSync(path.join(dir, 'comparison.json')));
    assert.ok(fs.existsSync(path.join(dir, 'summary.md')));
  });

  it('sends only history series with --history', () => {
    const { dir } = writeResults('cli-history', [...everyTarget(0), ...everyTarget(1)]);
    const result = spawnSync(
      process.execPath,
      [COMPARE, '--results', dir, '--dry-run', '--history', '--history-run-id', '42'],
      {
        encoding: 'utf8',
        env: { ...process.env, GITHUB_STEP_SUMMARY: '', GITHUB_ACTIONS: '' }
      }
    );
    assert.equal(result.status, 0, result.stderr);
    const { series } = JSON.parse(result.stdout.slice(result.stdout.indexOf('{\n  "series"')));
    assert.ok(series.length > 0);
    for (const s of series) {
      assert.match(s.metric, /^electron\.perf\.history\./);
      assert.ok(s.tags.includes('history_run_id:42'));
      assert.ok(s.tags.includes('electron_major:47'));
    }
  });
});
