// PGO profile collection orchestrator.
//
// Runs against an *instrumented* Electron build (built with
// chrome_pgo_phase = 1) and produces a merged .profdata file ready for use in
// release builds via the pgo_data_path GN arg.
//
//   node script/pgo/collect-profile.js \
//     --electron <path to instrumented electron binary> \
//     --output <path to write merged .profdata> \
//     [--work-dir <scratch dir>] [--port 8765] [--no-sandbox]
//
// --no-sandbox is required when running as root inside CI containers; the
// sandbox setup paths are a negligible fraction of profile counters.
//
// The flow mirrors Chromium's tools/pgo/generate_profile.py:
//   1. Serve the benchmark workloads locally (Speedometer 3, JetStream 2,
//      MotionMark).
//   2. Launch the instrumented build with LLVM_PROFILE_FILE set; the
//      benchmark-app drives every workload and quits itself, giving every
//      Electron process a clean shutdown so its counters are written.
//   3. Merge all .profraw files with llvm-profdata into one .profdata.
//
// Display requirements: the caller is responsible for providing a display
// (Xvfb on Linux CI; native sessions on Windows/macOS runners).
const { spawn, execFileSync } = require('node:child_process');
const fs = require('node:fs');
const os = require('node:os');
const path = require('node:path');

const { fetchBenchmarks, startServer } = require('./serve-benchmarks');

const SRC_DIR = path.resolve(__dirname, '..', '..', '..');

function parseArgs (argv) {
  const args = {};
  for (let i = 2; i < argv.length; i++) {
    if (argv[i].startsWith('--')) {
      const key = argv[i].slice(2);
      args[key] = argv[i + 1] && !argv[i + 1].startsWith('--') ? argv[++i] : true;
    }
  }
  return args;
}

function log (...args) {
  // eslint-disable-next-line no-console
  console.log('[collect-profile]', ...args);
}

// llvm-profdata ships in the clang "coverage tools" package, which is not part
// of the default toolchain download. IMPORTANT: it must be extracted to its
// own directory - extracting a package into third_party/llvm-build replaces
// the existing clang installation.
function ensureLlvmProfdata () {
  const exeSuffix = process.platform === 'win32' ? '.exe' : '';
  const coverageToolsDir = path.join(SRC_DIR, 'third_party', 'llvm-coverage-tools');
  const profdataPath = path.join(coverageToolsDir, 'Release+Asserts', 'bin', `llvm-profdata${exeSuffix}`);
  if (fs.existsSync(profdataPath)) return profdataPath;

  log('downloading llvm coverage tools (llvm-profdata)');
  execFileSync('python3', [
    path.join(SRC_DIR, 'tools', 'clang', 'scripts', 'update.py'),
    '--package=coverage_tools',
    `--output-dir=${coverageToolsDir}`
  ], { stdio: 'inherit' });

  if (!fs.existsSync(profdataPath)) {
    throw new Error(`llvm-profdata not found at ${profdataPath} after download`);
  }
  return profdataPath;
}

async function main () {
  const args = parseArgs(process.argv);
  if (!args.electron || !args.output) {
    console.error('Usage: collect-profile.js --electron <binary> --output <profdata> [--work-dir <dir>] [--port <port>]');
    process.exit(1);
  }

  const electronBinary = path.resolve(args.electron);
  const outputPath = path.resolve(args.output);
  const workDir = path.resolve(args['work-dir'] || path.join(os.tmpdir(), 'electron-pgo'));
  const port = parseInt(args.port || '8765', 10);
  const profrawDir = path.join(workDir, 'profraw');
  const benchmarksDir = path.join(workDir, 'benchmarks');

  if (!fs.existsSync(electronBinary)) {
    throw new Error(`instrumented electron binary not found: ${electronBinary}`);
  }
  fs.mkdirSync(profrawDir, { recursive: true });
  fs.mkdirSync(benchmarksDir, { recursive: true });

  // 1. Benchmarks.
  fetchBenchmarks(benchmarksDir);
  const server = startServer(benchmarksDir, port);

  // 2. Run the instrumented build through the workloads.
  //    %m = profile merge pools: multiple processes share pool files and merge
  //    counters atomically, keeping the file count manageable.
  const profilePattern = path.join(profrawDir, 'electron-%4m.profraw');
  const resultsFile = path.join(workDir, 'benchmark-results.json');

  log(`launching instrumented electron: ${electronBinary}`);
  const electronArgs = [
    path.join(__dirname, 'benchmark-app'),
    // Benchmark content is served from localhost; the renderer needs no
    // network access beyond that.
    '--disable-background-networking'
  ];
  if (args['no-sandbox']) electronArgs.push('--no-sandbox');

  const startTime = Date.now();
  const exitCode = await new Promise((resolve, reject) => {
    const child = spawn(electronBinary, electronArgs, {
      env: {
        ...process.env,
        LLVM_PROFILE_FILE: profilePattern,
        PGO_BENCHMARK_BASE_URL: `http://127.0.0.1:${port}`,
        PGO_RESULTS_FILE: resultsFile
      },
      stdio: 'inherit'
    });
    child.on('error', reject);
    child.on('exit', (code) => resolve(code ?? 1));
  });
  server.close();

  const elapsedMin = ((Date.now() - startTime) / 60000).toFixed(1);
  log(`instrumented run finished with exit code ${exitCode} in ${elapsedMin} minutes`);
  if (fs.existsSync(resultsFile)) {
    log(`workload results:\n${fs.readFileSync(resultsFile, 'utf8')}`);
  }

  // 3. Merge.
  const profrawFiles = fs.readdirSync(profrawDir)
    .filter(f => f.endsWith('.profraw'))
    .map(f => path.join(profrawDir, f));
  if (profrawFiles.length === 0) {
    throw new Error(
      'no .profraw files were written - the instrumented build did not shut ' +
      'down cleanly, or the binary was not built with chrome_pgo_phase = 1'
    );
  }
  log(`merging ${profrawFiles.length} profraw files`);

  const llvmProfdata = ensureLlvmProfdata();
  fs.mkdirSync(path.dirname(outputPath), { recursive: true });
  execFileSync(llvmProfdata, ['merge', '-o', outputPath, ...profrawFiles], { stdio: 'inherit' });

  const sizeMB = (fs.statSync(outputPath).size / 1024 / 1024).toFixed(1);
  log(`wrote ${outputPath} (${sizeMB} MB)`);

  // A failed workload produces a usable but lower-coverage profile; surface it
  // as a job failure so CI runs are investigated, while still keeping the
  // merged output for debugging.
  process.exit(exitCode);
}

main().catch((err) => {
  console.error('[collect-profile] FATAL:', err);
  process.exit(1);
});
