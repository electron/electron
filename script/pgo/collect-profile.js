// PGO profile collection orchestrator.
//
// Runs against an *instrumented* Electron build (built with
// chrome_pgo_phase = 1) and produces a merged .profdata file ready for use in
// release builds via the pgo_data_path GN arg.
//
//   node script/pgo/collect-profile.js \
//     --electron <path to instrumented electron binary> \
//     --output <path to write merged .profdata> \
//     [--work-dir <scratch dir>] [--port 8765] [--no-sandbox] [--no-merge]
//
// --no-sandbox is required when running as root inside CI containers; the
// sandbox setup paths are a negligible fraction of profile counters.
//
// --no-merge skips the llvm-profdata merge and instead copies the raw
// .profraw files into --output (treated as a directory). Use this on hosts
// that cannot run llvm-profdata (no Chromium checkout, or non-x64 hosts like
// the Linux arm64 CI runners) - .profraw files are arch-independent data and
// can be merged later on any machine with a matching llvm-profdata (see
// fetch-llvm-profdata.js).
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

const { fetchBenchmarks, startServer, generateCerts } = require('./serve-benchmarks');

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

// Installs the ephemeral collection CA into the host's trust store so the
// instrumented build exercises the real certificate verification path during
// the network workloads. A bypass flag (--ignore-certificate-errors) would
// train the wrong branch - verification short-circuits before the chain
// building / signature checking / trust lookup code that real apps run on
// every connection.
//
// Trust installation is per-platform:
//   linux  - Chromium reads the NSS user DB (certutil from libnss3-tools)
//   darwin - system keychain (runners have passwordless sudo)
//   win32  - machine ROOT store (runners are administrators)
//
// Returns true on success. On failure the caller falls back to plain HTTP -
// network workload coverage degrades but collection still succeeds.
function installTrustedCA (caCertPath) {
  try {
    if (process.platform === 'linux') {
      const nssDb = path.join(os.homedir(), '.pki', 'nssdb');
      if (!fs.existsSync(nssDb)) {
        fs.mkdirSync(nssDb, { recursive: true });
        execFileSync('certutil', ['-d', `sql:${nssDb}`, '-N', '--empty-password'], { stdio: 'pipe' });
      }
      execFileSync('certutil', [
        '-d', `sql:${nssDb}`, '-A', '-t', 'C,,', '-n', 'Electron PGO Ephemeral CA', '-i', caCertPath
      ], { stdio: 'pipe' });
    } else if (process.platform === 'darwin') {
      execFileSync('sudo', [
        'security', 'add-trusted-cert', '-d', '-r', 'trustRoot',
        '-k', '/Library/Keychains/System.keychain', caCertPath
      ], { stdio: 'pipe' });
    } else if (process.platform === 'win32') {
      execFileSync('certutil', ['-addstore', '-f', 'ROOT', caCertPath], { stdio: 'pipe' });
    } else {
      return false;
    }
    log('collection CA installed into the host trust store');
    return true;
  } catch (err) {
    log(`WARNING: could not install collection CA (${err.message}); network workloads fall back to HTTP`);
    return false;
  }
}

// llvm-profdata ships in the clang "coverage tools" package, which is not part
// of the default toolchain download. IMPORTANT: it must be extracted to its
// own directory - extracting a package into third_party/llvm-build replaces
// the existing clang installation.
function ensureLlvmProfdata () {
  const exeSuffix = process.platform === 'win32' ? '.exe' : '';
  const coverageToolsDir = path.join(SRC_DIR, 'third_party', 'llvm-coverage-tools');
  const profdataPath = path.join(coverageToolsDir, 'bin', `llvm-profdata${exeSuffix}`);
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

  // 1. Benchmarks, TLS material, and the local server.
  //
  //    The server prefers HTTPS + HTTP/2 with a CA installed into the host
  //    trust store, so resource loads and the network workloads train
  //    Chromium's TLS handshake, certificate verification, and H2 session
  //    code. If cert generation or trust installation fails, collection falls
  //    back to plain HTTP (lower network coverage, everything else intact).
  fetchBenchmarks(benchmarksDir);
  let tls = null;
  let caCertPath = null;
  const certs = generateCerts(path.join(workDir, 'certs'));
  if (certs && installTrustedCA(certs.caCertPath)) {
    tls = { certPath: certs.certPath, keyPath: certs.keyPath };
    caCertPath = certs.caCertPath;
  }
  const server = startServer(benchmarksDir, port, { tls });
  const baseUrl = tls ? `https://localhost:${port}` : `http://127.0.0.1:${port}`;

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
    '--disable-background-networking',
    // Route Chromium/V8 logging to stderr so CI logs capture pre-crash
    // diagnostics - official builds otherwise crash without any message.
    '--enable-logging=stderr'
  ];
  if (args['no-sandbox']) electronArgs.push('--no-sandbox');

  const startTime = Date.now();
  let exitCode = await new Promise((resolve, reject) => {
    const child = spawn(electronBinary, electronArgs, {
      env: {
        ...process.env,
        LLVM_PROFILE_FILE: profilePattern,
        PGO_BENCHMARK_BASE_URL: baseUrl,
        PGO_RESULTS_FILE: resultsFile,
        // Node's TLS stack (used by the main-process network workload) does
        // not read the OS trust store; point it at the collection CA.
        ...(caCertPath ? { NODE_EXTRA_CA_CERTS: caCertPath } : {})
      },
      stdio: 'inherit'
    });
    child.on('error', reject);
    child.on('exit', (code, signal) => {
      // A signal exit (code === null) usually means the OOM killer (SIGKILL)
      // or a crash (SIGSEGV/SIGABRT) - log it so CI failures are diagnosable.
      if (signal) log(`electron was killed by signal ${signal}`);
      resolve(code ?? 1);
    });
  });
  server.close();

  const elapsedMin = ((Date.now() - startTime) / 60000).toFixed(1);
  log(`instrumented run finished with exit code ${exitCode} in ${elapsedMin} minutes`);

  // Electron's clean-shutdown path (app.quit()) always exits 0, so workload
  // failures are reported through the results file rather than the exit code.
  let failedWorkloads = [];
  if (fs.existsSync(resultsFile)) {
    const results = JSON.parse(fs.readFileSync(resultsFile, 'utf8'));
    log(`workload results:\n${JSON.stringify(results, null, 2)}`);
    failedWorkloads = results.filter(r => !r.ok);
  } else {
    log('WARNING: no results file was written - the app may have crashed');
    failedWorkloads = [{ name: 'all', error: 'results file missing' }];
  }
  if (exitCode === 0 && failedWorkloads.length > 0) {
    exitCode = 1;
  }

  // 3. Merge (or hand off the raw files for later merging).
  const profrawFiles = fs.readdirSync(profrawDir)
    .filter(f => f.endsWith('.profraw'))
    .map(f => path.join(profrawDir, f));
  if (profrawFiles.length === 0) {
    throw new Error(
      'no .profraw files were written - the instrumented build did not shut ' +
      'down cleanly, or the binary was not built with chrome_pgo_phase = 1'
    );
  }

  if (args['no-merge']) {
    fs.mkdirSync(outputPath, { recursive: true });
    for (const file of profrawFiles) {
      fs.copyFileSync(file, path.join(outputPath, path.basename(file)));
    }
    log(`copied ${profrawFiles.length} profraw files to ${outputPath} (merge skipped)`);
  } else {
    log(`merging ${profrawFiles.length} profraw files`);
    const llvmProfdata = ensureLlvmProfdata();
    fs.mkdirSync(path.dirname(outputPath), { recursive: true });
    execFileSync(llvmProfdata, ['merge', '-o', outputPath, ...profrawFiles], { stdio: 'inherit' });
    const sizeMB = (fs.statSync(outputPath).size / 1024 / 1024).toFixed(1);
    log(`wrote ${outputPath} (${sizeMB} MB)`);
  }

  // A failed workload produces a usable but lower-coverage profile; surface it
  // as a job failure so CI runs are investigated, while still keeping the
  // collected output (the CI artifact upload runs even when this step fails).
  if (failedWorkloads.length > 0) {
    log(`FAILED workloads: ${failedWorkloads.map(w => w.name).join(', ')}`);
  }
  process.exit(exitCode);
}

main().catch((err) => {
  console.error('[collect-profile] FATAL:', err);
  process.exit(1);
});
