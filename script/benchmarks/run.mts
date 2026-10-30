// Runs the benchmark suites against every target in targets.json (written by
// fetch-targets.mts), interleaved: each round runs every suite once per
// target (A, B, C, then B, C, A, ...) so slow drift on the host affects all
// targets equally. Results land in <out>/rounds/<round>/<suite>/<target>/ and
// <out>/metadata.json; compare.mts turns them into a comparison.
//
// Usage (under Xvfb on Linux):
//   node script/benchmarks/run.mts --targets <dir>/targets.json --out <dir>
//     [--rounds 10] [--repeat 3] [--suites loading,trace,file_loading,speedometer]
//     [--file-count 1500] [--work-dir <dir>] [--python python3]
import { type ChildProcess, execFile, execFileSync, spawn } from 'node:child_process';
import * as fs from 'node:fs';
import * as os from 'node:os';
import * as path from 'node:path';
import { fileURLToPath } from 'node:url';
import { parseArgs } from 'node:util';

import { pageConfigs, probeConfigs, writeFileLoadingSite } from './lib/configs.mts';

import type { Capabilities, Invocation, RunMetadata, Target, TargetsFile } from './lib/types.mts';

const SERVE_BENCHMARKS = path.resolve(import.meta.dirname, '../pgo/serve-benchmarks.js');

// The one place crossbench is pinned. Installed from a plain checkout (not
// `pip install git+...`, which would try to clone Google-internal submodules)
// as an editable package, because crossbench imports its top-level
// third_party/ directory at runtime.
export const CROSSBENCH_REPO = 'https://chromium.googlesource.com/crossbench';
export const CROSSBENCH_REVISION = '18eb2497c6ab1d130e40512187a03a5c0cc79255';

export const SUITES = ['loading', 'trace', 'file_loading', 'speedometer', 'jetstream'];

// Suites driven by generated page configs and crossbench's `loading` benchmark.
const PAGE_SUITES = ['loading', 'trace', 'file_loading'];

// Press benchmarks are served from the same pinned WebKit checkouts PGO uses.
// MotionMark is left out: the revision PGO pins no longer exposes the
// window.benchmarkRunnerClient global crossbench's MotionMark probe reads.
const PRESS_BENCHMARKS: Record<string, { benchmark: string; path: string }> = {
  speedometer: { benchmark: 'speedometer_3.1', path: 'speedometer/' },
  jetstream: { benchmark: 'jetstream_2.2', path: 'jetstream/' }
};

function ensureCrossbench(workDir: string, python: string): string {
  const checkout = path.join(workDir, 'crossbench');
  const venv = path.join(workDir, 'crossbench-venv');
  const cb = path.join(venv, 'bin', 'cb');
  const git = (...args: string[]) => execFileSync('git', args, { cwd: checkout, encoding: 'utf8' }).trim();
  if (
    fs.existsSync(cb) &&
    fs.existsSync(path.join(checkout, '.git')) &&
    git('rev-parse', 'HEAD') === CROSSBENCH_REVISION
  ) {
    return cb;
  }
  console.log(`[run] installing crossbench @ ${CROSSBENCH_REVISION}`);
  fs.rmSync(checkout, { recursive: true, force: true });
  fs.rmSync(venv, { recursive: true, force: true });
  fs.mkdirSync(checkout, { recursive: true });
  git('init', '--quiet');
  git('remote', 'add', 'origin', CROSSBENCH_REPO);
  git('fetch', '--quiet', '--depth', '1', 'origin', CROSSBENCH_REVISION);
  git('checkout', '--quiet', 'FETCH_HEAD');
  execFileSync(python, ['-m', 'venv', venv], { stdio: 'inherit' });
  execFileSync(path.join(venv, 'bin', 'pip'), ['install', '--quiet', '--editable', checkout], { stdio: 'inherit' });
  return cb;
}

// Starts script/pgo/serve-benchmarks.js over <work>/www, which also fetches
// the pinned Speedometer, JetStream and MotionMark checkouts on first use.
async function startServer(wwwDir: string, port: number): Promise<ChildProcess> {
  const server = spawn(process.execPath, [SERVE_BENCHMARKS, '--dir', wwwDir, '--port', String(port)], {
    stdio: ['ignore', 'inherit', 'inherit']
  });
  const deadline = Date.now() + 10 * 60_000;
  while (Date.now() < deadline) {
    if (server.exitCode !== null) throw new Error(`serve-benchmarks.js exited with ${server.exitCode}`);
    try {
      const response = await fetch(`http://127.0.0.1:${port}/pages/index.html`);
      if (response.ok) return server;
    } catch {
      // Not listening yet.
    }
    await new Promise((resolve) => setTimeout(resolve, 1000));
  }
  server.kill();
  throw new Error('Timed out waiting for serve-benchmarks.js');
}

// Asks the harness app which features this Electron version supports.
function probeCapabilities(target: Target, harnessApp: string, workDir: string): Capabilities {
  const file = path.join(workDir, `${target.id}.capabilities.json`);
  fs.rmSync(file, { force: true });
  try {
    execFileSync(target.binary, ['--no-sandbox', '--headless', harnessApp, `--cb-capabilities=${file}`], {
      stdio: 'inherit',
      timeout: 60_000
    });
    return JSON.parse(fs.readFileSync(file, 'utf8'));
  } catch (error) {
    console.warn(
      `[run] could not probe ${target.id} capabilities (${errorMessage(error)}); running the basic stories only`
    );
    return {};
  }
}

async function readAzureVmSize(): Promise<string | undefined> {
  try {
    const response = await fetch(
      'http://169.254.169.254/metadata/instance/compute/vmSize?api-version=2021-02-01&format=text',
      {
        headers: { Metadata: 'true' },
        signal: AbortSignal.timeout(2000)
      }
    );
    return response.ok ? (await response.text()).trim() : undefined;
  } catch {
    return undefined;
  }
}

// Host details recorded with every run, so results from different hardware
// (for example an ARC fallback to an older VM generation) can be told apart.
async function hostInfo() {
  let cpuModel = os.cpus()[0]?.model?.trim();
  try {
    cpuModel =
      fs
        .readFileSync('/proc/cpuinfo', 'utf8')
        .match(/^model name\s*:\s*(.+)$/m)?.[1]
        ?.trim() ?? cpuModel;
  } catch {
    // Not Linux.
  }
  return {
    platform: process.platform,
    arch: process.arch,
    cpuModel: cpuModel ?? 'unknown',
    cpuCount: os.availableParallelism(),
    memoryGiB: Math.round(os.totalmem() / 2 ** 30),
    kernel: `${os.type()} ${os.release()}`,
    runnerName: process.env.RUNNER_NAME ?? os.hostname(),
    vmSize: await readAzureVmSize()
  };
}

const errorMessage = (error: unknown) => (error instanceof Error ? error.message : String(error));

const sleep = (ms: number) => new Promise((resolve) => setTimeout(resolve, ms));

// Whether any process in the group still runs. Zombies (killed descendants
// that were reparented to an init that has not reaped them yet) count as gone.
function groupAlive(pgid: number): boolean {
  try {
    const members = fs
      .readdirSync('/proc')
      .filter((entry) => /^\d+$/.test(entry))
      .map((pid) => {
        try {
          // The fields after the parenthesized command are: state ppid pgrp.
          const stat = fs.readFileSync(`/proc/${pid}/stat`, 'utf8');
          return stat.slice(stat.lastIndexOf(')') + 2).split(' ');
        } catch {
          return [];
        }
      });
    return members.some(([state, , pgrp]) => Number(pgrp) === pgid && state !== 'Z');
  } catch {
    // No /proc (macOS): fall back to signal 0, which also sees zombies.
    try {
      process.kill(-pgid, 0);
      return true;
    } catch {
      return false;
    }
  }
}

function signalGroup(pgid: number, signal: NodeJS.Signals) {
  try {
    process.kill(-pgid, signal);
  } catch {
    // ESRCH: the group is already gone.
  }
}

// Kills the whole process tree of one invocation (crossbench, chromedriver and
// the browser), SIGTERM first and SIGKILL after graceMs, and waits until it is
// gone so the next target never shares the machine with leftovers.
async function killTree(child: ChildProcess, graceMs: number): Promise<void> {
  if (process.platform === 'win32') {
    // No process groups. taskkill /T walks the parent links, so it is only
    // used while crossbench itself is still alive (on timeout).
    await new Promise<void>((resolve) =>
      execFile('taskkill', ['/T', '/F', '/PID', String(child.pid)], { windowsHide: true }, () => resolve())
    );
    return;
  }
  const pgid = child.pid;
  if (pgid === undefined || !groupAlive(pgid)) return;
  signalGroup(pgid, 'SIGTERM');
  const deadline = Date.now() + graceMs;
  while (groupAlive(pgid) && Date.now() < deadline) await sleep(100);
  if (!groupAlive(pgid)) return;
  signalGroup(pgid, 'SIGKILL');
  const killDeadline = Date.now() + 10_000;
  while (groupAlive(pgid) && Date.now() < killDeadline) await sleep(100);
  if (groupAlive(pgid)) console.warn(`[run] processes in group ${pgid} survived SIGKILL`);
}

// Invocations in flight, killed if run.mts itself is stopped.
const activeChildren = new Set<ChildProcess>();
// Called with the signal name before run.mts exits on a signal.
let onStopSignal: (signal: NodeJS.Signals) => void = () => {};

// crossbench runs in its own process group, so a Ctrl+C or SIGTERM sent to
// run.mts would not reach it. Take its process tree down too.
function killActiveChildrenOnSignals() {
  for (const signal of ['SIGINT', 'SIGTERM'] as const) {
    process.on(signal, () => {
      for (const child of activeChildren) {
        if (process.platform === 'win32') {
          child.kill();
        } else if (child.pid !== undefined) {
          signalGroup(child.pid, 'SIGKILL');
        }
      }
      onStopSignal(signal);
      process.exit(128 + os.constants.signals[signal]);
    });
  }
}

// Runs one crossbench invocation and resolves with its exit code once it and
// all of its descendants are gone. On POSIX crossbench gets its own process
// group (detached), so the browser and chromedriver it starts can be killed
// with it. A timeout returns 124, death by a signal 128 + the signal number.
export async function runCrossbench(
  cb: string,
  args: string[],
  logFile: string,
  timeoutMs: number,
  { graceMs = 10_000 }: { graceMs?: number } = {}
): Promise<number> {
  const log = fs.openSync(logFile, 'w');
  let child: ChildProcess;
  try {
    child = spawn(cb, args, { stdio: ['ignore', log, log], detached: process.platform !== 'win32' });
  } finally {
    fs.closeSync(log);
  }
  activeChildren.add(child);
  let killing: Promise<void> | undefined;
  const exited = new Promise<{ code: number | null; signal: NodeJS.Signals | null }>((resolve) => {
    child.on('error', (error) => {
      console.warn(`[run] could not start crossbench: ${error.message}`);
      resolve({ code: 1, signal: null });
    });
    child.on('exit', (code, signal) => resolve({ code, signal }));
  });
  const timer = setTimeout(() => {
    console.warn(`[run] crossbench timed out after ${timeoutMs / 1000}s; killing its process tree`);
    killing = killTree(child, graceMs);
  }, timeoutMs);
  const { code, signal } = await exited;
  clearTimeout(timer);
  const timedOut = killing !== undefined;
  if (killing) {
    await killing;
  } else if (child.pid !== undefined && process.platform !== 'win32') {
    // Also after a normal exit: nothing crossbench started may outlive it.
    // (Not on Windows, where the PID may already belong to another process.)
    await killTree(child, graceMs);
  }
  activeChildren.delete(child);
  if (timedOut) return 124;
  if (signal) return 128 + (os.constants.signals[signal] ?? 0);
  return code ?? 1;
}

async function main() {
  killActiveChildrenOnSignals();
  const { values: args } = parseArgs({
    options: {
      targets: { type: 'string' },
      out: { type: 'string' },
      rounds: { type: 'string', default: '10' },
      repeat: { type: 'string', default: '3' },
      suites: { type: 'string', default: 'loading,trace,file_loading,speedometer' },
      'file-count': { type: 'string', default: '1500' },
      'work-dir': { type: 'string', default: path.join(os.tmpdir(), 'electron-benchmarks') },
      python: { type: 'string', default: 'python3' },
      port: { type: 'string', default: '8765' },
      'timeout-minutes': { type: 'string', default: '30' }
    }
  });
  if (!args.targets || !args.out) throw new Error('--targets and --out are required');
  const rounds = parseInt(args.rounds, 10);
  const repeat = parseInt(args.repeat, 10);
  const fileCount = parseInt(args['file-count'], 10);
  const port = parseInt(args.port, 10);
  const suites = args.suites.split(',').map((suite) => suite.trim());
  for (const suite of suites) {
    if (!SUITES.includes(suite)) throw new Error(`Unknown suite ${suite}; expected one of ${SUITES.join(', ')}`);
  }
  if (!(rounds > 0 && repeat > 0 && fileCount > 0)) {
    throw new Error('--rounds, --repeat and --file-count must be positive');
  }

  const outDir = path.resolve(args.out);
  const workDir = path.resolve(args['work-dir']);
  fs.mkdirSync(outDir, { recursive: true });
  fs.mkdirSync(workDir, { recursive: true });
  const { targets } = JSON.parse(fs.readFileSync(args.targets, 'utf8')) as TargetsFile;
  const harnessApp = path.join(import.meta.dirname, 'harness-app');

  const cb = ensureCrossbench(workDir, args.python);

  const wwwDir = path.join(workDir, 'www');
  fs.cpSync(path.join(import.meta.dirname, 'pages'), path.join(wwwDir, 'pages'), { recursive: true });
  const fileRoot = path.join(workDir, 'file-loading');
  writeFileLoadingSite(fileRoot, fileCount);
  const server = await startServer(wwwDir, port);
  const baseUrl = `http://127.0.0.1:${port}`;

  const configDir = path.join(workDir, 'configs');
  fs.mkdirSync(configDir, { recursive: true });
  const probeConfigPaths: Record<string, string> = {};
  for (const [suite, config] of Object.entries(probeConfigs())) {
    probeConfigPaths[suite] = path.join(configDir, `${suite}.probes.json`);
    fs.writeFileSync(probeConfigPaths[suite], JSON.stringify(config, null, 2));
  }
  const capabilities = new Map<string, Capabilities>();
  const pageConfigPaths = new Map<string, Record<string, string>>();
  for (const target of targets) {
    const targetCapabilities = target.kind === 'electron' ? probeCapabilities(target, harnessApp, workDir) : {};
    capabilities.set(target.id, targetCapabilities);
    const configs = pageConfigs({ kind: target.kind, capabilities: targetCapabilities, baseUrl, fileRoot });
    const paths: Record<string, string> = {};
    for (const [suite, config] of Object.entries(configs)) {
      if (Object.keys(config.pages).length === 0) continue;
      paths[suite] = path.join(configDir, `${target.id}.${suite}.pages.json`);
      fs.writeFileSync(paths[suite], JSON.stringify(config, null, 2));
    }
    pageConfigPaths.set(target.id, paths);
  }

  const metadata: RunMetadata = {
    startedAt: new Date().toISOString(),
    ...(await hostInfo()),
    crossbenchRevision: CROSSBENCH_REVISION,
    rounds,
    repeat,
    suites,
    fileCount,
    targets: targets.map(({ id, kind, electronVersion, chromiumVersion }) => ({
      id,
      kind,
      electronVersion,
      chromiumVersion,
      capabilities: capabilities.get(id)
    })),
    invocations: [] as Invocation[]
  };
  const writeMetadata = () => fs.writeFileSync(path.join(outDir, 'metadata.json'), JSON.stringify(metadata, null, 2));
  onStopSignal = (signal) => {
    metadata.finishedAt = new Date().toISOString();
    metadata.stoppedBy = signal;
    writeMetadata();
  };
  console.log(
    `[run] host: ${metadata.cpuModel}, ${metadata.cpuCount} CPUs, ${metadata.kernel}, ${metadata.runnerName}`
  );

  try {
    for (let round = 0; round < rounds; round++) {
      // Rotate the target order every round so no target always runs first.
      const order = targets.map((_, i) => targets[(i + round) % targets.length]);
      for (const suite of suites) {
        for (const target of order) {
          const suiteArgs: string[] = [];
          // Values each measure should have, checked by compare.mts. The
          // press benchmarks run once per invocation (crossbench's default).
          let repetitions = 1;
          if (PAGE_SUITES.includes(suite)) {
            repetitions = repeat;
            const pageConfig = pageConfigPaths.get(target.id)?.[suite];
            if (!pageConfig) continue;
            suiteArgs.push(
              'loading',
              `--page-config=${pageConfig}`,
              `--probe-config=${probeConfigPaths[suite]}`,
              '--separate',
              `--repeat=${repeat}`
            );
          } else {
            const press = PRESS_BENCHMARKS[suite];
            suiteArgs.push(press.benchmark, `--url=${baseUrl}/${press.path}`);
          }
          const relativeDir = path.join('rounds', String(round), suite, target.id);
          const resultDir = path.join(outDir, relativeDir);
          fs.mkdirSync(path.dirname(resultDir), { recursive: true });
          const cbArgs = [
            ...suiteArgs,
            `--browser-config=${target.browserConfig}`,
            // Electron does not implement Browser.getWindowForTarget, which
            // crossbench's headed window setup needs.
            '--headless',
            '--env-validation=skip',
            `--out-dir=${resultDir}`
          ];
          const started = Date.now();
          console.log(`[run] round ${round + 1}/${rounds} ${suite} ${target.id}`);
          const exitCode = await runCrossbench(
            cb,
            cbArgs,
            `${resultDir}.log`,
            parseInt(args['timeout-minutes'], 10) * 60_000
          );
          const seconds = Math.round((Date.now() - started) / 100) / 10;
          if (exitCode !== 0) {
            console.warn(`[run] crossbench exited with ${exitCode} after ${seconds}s, see ${resultDir}.log`);
          }
          metadata.invocations.push({
            round,
            suite,
            target: target.id,
            exitCode,
            repetitions,
            seconds,
            dir: relativeDir
          });
          writeMetadata();
        }
      }
    }
  } finally {
    server.kill();
    metadata.finishedAt = new Date().toISOString();
    writeMetadata();
  }

  // Policy: any failed invocation fails the run. Every round still runs and
  // metadata.json lists each invocation's exit code, so the results that did
  // succeed can still be compared and uploaded as diagnostics.
  const failed = metadata.invocations.filter((invocation) => invocation.exitCode !== 0).length;
  console.log(`[run] ${metadata.invocations.length} crossbench runs, ${failed} failed`);
  if (failed > 0) {
    console.error(`ERROR: ${failed} of ${metadata.invocations.length} crossbench runs failed`);
    process.exitCode = 1;
  }
}

if (process.argv[1] && fs.realpathSync(process.argv[1]) === fileURLToPath(import.meta.url)) {
  main().catch((error: unknown) => {
    console.error(`ERROR: ${errorMessage(error)}`);
    process.exit(1);
  });
}
