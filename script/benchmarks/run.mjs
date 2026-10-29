// Runs the benchmark suites against every target in targets.json (written by
// fetch-targets.mjs), interleaved: each round runs every suite once per
// target (A, B, C, then B, C, A, ...) so slow drift on the host affects all
// targets equally. Results land in <out>/rounds/<round>/<suite>/<target>/ and
// <out>/metadata.json; compare.mjs turns them into a comparison.
//
// Usage (under Xvfb on Linux):
//   node script/benchmarks/run.mjs --targets <dir>/targets.json --out <dir>
//     [--rounds 10] [--repeat 3] [--suites loading,file_loading,speedometer]
//     [--file-count 1500] [--work-dir <dir>] [--python python3]
import { execFileSync, spawn } from 'node:child_process';
import * as fs from 'node:fs';
import * as os from 'node:os';
import * as path from 'node:path';
import { fileURLToPath } from 'node:url';
import { parseArgs } from 'node:util';

import { pageConfigs, probeConfig, writeFileLoadingSite } from './lib/configs.mjs';

const __dirname = path.dirname(fileURLToPath(import.meta.url));
const SERVE_BENCHMARKS = path.resolve(__dirname, '../pgo/serve-benchmarks.js');

// The one place crossbench is pinned. Installed from a plain checkout (not
// `pip install git+...`, which would try to clone Google-internal submodules)
// as an editable package, because crossbench imports its top-level
// third_party/ directory at runtime.
export const CROSSBENCH_REPO = 'https://chromium.googlesource.com/crossbench';
export const CROSSBENCH_REVISION = '18eb2497c6ab1d130e40512187a03a5c0cc79255';

export const SUITES = ['loading', 'file_loading', 'speedometer', 'jetstream'];

// Press benchmarks are served from the same pinned WebKit checkouts PGO uses.
// MotionMark is left out: the revision PGO pins no longer exposes the
// window.benchmarkRunnerClient global crossbench's MotionMark probe reads.
const PRESS_BENCHMARKS = {
  speedometer: { benchmark: 'speedometer_3.1', path: 'speedometer/' },
  jetstream: { benchmark: 'jetstream_2.2', path: 'jetstream/' }
};

function ensureCrossbench(workDir, python) {
  const checkout = path.join(workDir, 'crossbench');
  const venv = path.join(workDir, 'crossbench-venv');
  const cb = path.join(venv, 'bin', 'cb');
  const git = (...args) => execFileSync('git', args, { cwd: checkout, encoding: 'utf8' }).trim();
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
async function startServer(wwwDir, port) {
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
function probeCapabilities(target, harnessApp, workDir) {
  const file = path.join(workDir, `${target.id}.capabilities.json`);
  fs.rmSync(file, { force: true });
  try {
    execFileSync(target.binary, ['--no-sandbox', '--headless', harnessApp, `--cb-capabilities=${file}`], {
      stdio: 'inherit',
      timeout: 60_000
    });
    return JSON.parse(fs.readFileSync(file, 'utf8'));
  } catch (error) {
    console.warn(`[run] could not probe ${target.id} capabilities (${error.message}); running the basic stories only`);
    return {};
  }
}

async function readAzureVmSize() {
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

function runCrossbench(cb, args, logFile, timeoutMs) {
  return new Promise((resolve) => {
    const log = fs.openSync(logFile, 'w');
    const child = spawn(cb, args, { stdio: ['ignore', log, log] });
    const timer = setTimeout(() => child.kill('SIGKILL'), timeoutMs);
    child.on('close', (code, signal) => {
      clearTimeout(timer);
      fs.closeSync(log);
      resolve(code ?? (signal ? 128 : 1));
    });
  });
}

async function main() {
  const { values: args } = parseArgs({
    options: {
      targets: { type: 'string' },
      out: { type: 'string' },
      rounds: { type: 'string', default: '10' },
      repeat: { type: 'string', default: '3' },
      suites: { type: 'string', default: 'loading,file_loading,speedometer' },
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
  const { targets } = JSON.parse(fs.readFileSync(args.targets, 'utf8'));
  const harnessApp = path.join(__dirname, 'harness-app');

  const cb = ensureCrossbench(workDir, args.python);

  const wwwDir = path.join(workDir, 'www');
  fs.cpSync(path.join(__dirname, 'pages'), path.join(wwwDir, 'pages'), { recursive: true });
  const fileRoot = path.join(workDir, 'file-loading');
  writeFileLoadingSite(fileRoot, fileCount);
  const server = await startServer(wwwDir, port);
  const baseUrl = `http://127.0.0.1:${port}`;

  const configDir = path.join(workDir, 'configs');
  fs.mkdirSync(configDir, { recursive: true });
  const probeConfigPath = path.join(configDir, 'probe.json');
  fs.writeFileSync(probeConfigPath, JSON.stringify(probeConfig(), null, 2));
  for (const target of targets) {
    target.capabilities = target.kind === 'electron' ? probeCapabilities(target, harnessApp, workDir) : {};
    const configs = pageConfigs({ kind: target.kind, capabilities: target.capabilities, baseUrl, fileRoot });
    target.pageConfigs = {};
    for (const [suite, config] of Object.entries(configs)) {
      if (Object.keys(config.pages).length === 0) continue;
      target.pageConfigs[suite] = path.join(configDir, `${target.id}.${suite}.pages.json`);
      fs.writeFileSync(target.pageConfigs[suite], JSON.stringify(config, null, 2));
    }
  }

  const metadata = {
    startedAt: new Date().toISOString(),
    ...(await hostInfo()),
    crossbenchRevision: CROSSBENCH_REVISION,
    rounds,
    repeat,
    suites,
    fileCount,
    targets: targets.map(({ id, kind, electronVersion, chromiumVersion, capabilities }) => ({
      id,
      kind,
      electronVersion,
      chromiumVersion,
      capabilities
    })),
    invocations: []
  };
  const writeMetadata = () => fs.writeFileSync(path.join(outDir, 'metadata.json'), JSON.stringify(metadata, null, 2));
  console.log(
    `[run] host: ${metadata.cpuModel}, ${metadata.cpuCount} CPUs, ${metadata.kernel}, ${metadata.runnerName}`
  );

  try {
    for (let round = 0; round < rounds; round++) {
      // Rotate the target order every round so no target always runs first.
      const order = targets.map((_, i) => targets[(i + round) % targets.length]);
      for (const suite of suites) {
        for (const target of order) {
          const suiteArgs = [];
          if (suite === 'loading' || suite === 'file_loading') {
            if (!target.pageConfigs[suite]) continue;
            suiteArgs.push(
              'loading',
              `--page-config=${target.pageConfigs[suite]}`,
              `--probe-config=${probeConfigPath}`,
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
          metadata.invocations.push({ round, suite, target: target.id, exitCode, seconds, dir: relativeDir });
          writeMetadata();
        }
      }
    }
  } finally {
    server.kill();
    metadata.finishedAt = new Date().toISOString();
    writeMetadata();
  }

  const failed = metadata.invocations.filter((invocation) => invocation.exitCode !== 0).length;
  console.log(`[run] ${metadata.invocations.length} crossbench runs, ${failed} failed`);
}

if (process.argv[1] && fs.realpathSync(process.argv[1]) === fileURLToPath(import.meta.url)) {
  main().catch((error) => {
    console.error(`ERROR: ${error.message}`);
    process.exit(1);
  });
}
