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
//     [--target-arch <arch>] [--attempts <n>] [--retry-deadline-minutes <n>]
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
// Failed collections are retried (up to --attempts, default 5) by wiping the
// profraw dir and relaunching the app from scratch, so a published profile
// only ever contains counters from one fully-successful run.
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

function parseArgs(argv) {
  const args = {};
  for (let i = 2; i < argv.length; i++) {
    if (argv[i].startsWith('--')) {
      const key = argv[i].slice(2);
      args[key] = argv[i + 1] && !argv[i + 1].startsWith('--') ? argv[++i] : true;
    }
  }
  return args;
}

function log(...args) {
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
function installTrustedCA(caCertPath) {
  try {
    if (process.platform === 'linux') {
      const xdgData = process.env.XDG_DATA_HOME || path.join(os.homedir(), '.local', 'share');
      const nssDb = fs.existsSync(path.join(xdgData, 'pki', 'nssdb'))
        ? path.join(xdgData, 'pki', 'nssdb')
        : path.join(os.homedir(), '.pki', 'nssdb');
      if (!fs.existsSync(nssDb)) {
        fs.mkdirSync(nssDb, { recursive: true });
        execFileSync('certutil', ['-d', `sql:${nssDb}`, '-N', '--empty-password'], {
          stdio: 'pipe',
        });
      }
      execFileSync(
        'certutil',
        ['-d', `sql:${nssDb}`, '-A', '-t', 'C,,', '-n', 'Electron PGO Ephemeral CA', '-i', caCertPath],
        { stdio: 'pipe' }
      );
    } else if (process.platform === 'darwin') {
      execFileSync(
        'sudo',
        [
          'security',
          'add-trusted-cert',
          '-d',
          '-r',
          'trustRoot',
          '-k',
          '/Library/Keychains/System.keychain',
          caCertPath
        ],
        { stdio: 'pipe' }
      );
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
// Generates the packaged-app fixture: an ASAR archive shaped like real
// packaged Electron apps. Two patterns coexist in production:
//   - bundled apps (the majority of large production apps): a few
//     multi-megabyte bundles produced by esbuild/webpack, plus resources
//   - unbundled apps (electron-builder defaults): many small modules
// The fixture contains both, plus resources, so the ASAR read paths (header
// parse, lookup, large extraction, small extraction) and V8 large-script
// parsing are all covered. Built here (plain Node - Electron's asar hooks
// are not active in this process) with @electron/asar from the
// @electron-ci/pgo workspace; the benchmark app only reads from it.
async function generateAsarFixture(workDir) {
  const asar = require('@electron/asar');

  const srcDir = path.join(workDir, 'asar-fixture-src');
  const archivePath = path.join(workDir, 'asar-fixture.asar');
  if (fs.existsSync(archivePath)) return archivePath;

  // --- Bundled-app pattern: webpack-style bundles. Each generated module is
  // a realistic mix of functions, classes, object literals and string data so
  // V8's parser and compiler do representative work when the bundle loads.
  const distDir = path.join(srcDir, 'dist');
  fs.mkdirSync(distDir, { recursive: true });

  const makeBundle = (name, moduleCount) => {
    const parts = ['// Generated webpack-style bundle for PGO training\n', '(() => {\n', 'var __modules__ = {\n'];
    for (let m = 0; m < moduleCount; m++) {
      // Each module is ~3KB - the realistic average for webpack module output.
      const fields = Array.from(
        { length: 24 },
        (_, f) =>
          `field_${f}: { label: 'Field ${f} of module ${m}', value: ${m * f}, enabled: ${f % 2 === 0}, validators: ['required', 'max:${f * 10}'] }`
      ).join(',\n    ');
      const handlers = Array.from(
        { length: 8 },
        (_, h) =>
          `  function handler_${h}(event, payload) {\n` +
          `    const result = { type: 'event_${h}', source: ${m}, items: [] };\n` +
          '    for (let i = 0; i < (payload && payload.length ? payload.length : 4); i++) {\n' +
          `      result.items.push({ index: i, score: i * ${h + 1} + ${m % 7}, tag: 'item-' + i });\n` +
          '    }\n' +
          '    return result;\n' +
          '  }'
      ).join('\n');
      parts.push(
        `${m}: (module, exports, __require__) => {\n` +
          `  const config = { id: ${m}, name: 'module-${m}', flags: { a: ${m % 2 === 0}, b: ${m % 3 === 0} }, weights: [${[m, m * 2, m * 3, m * 5].join(',')}] };\n` +
          `  const schema = {\n    ${fields}\n  };\n` +
          `${handlers}\n` +
          '  function transform(input) {\n' +
          '    let acc = 0;\n' +
          '    for (let i = 0; i < config.weights.length; i++) acc += config.weights[i] * (input + i);\n' +
          '    return acc;\n' +
          '  }\n' +
          '  class Service {\n' +
          '    constructor() { this.cache = new Map(); this.hits = 0; }\n' +
          '    resolve(key) {\n' +
          '      if (this.cache.has(key)) { this.hits++; return this.cache.get(key); }\n' +
          "      const value = transform(key) + '-' + config.name;\n" +
          '      this.cache.set(key, value);\n' +
          '      return value;\n' +
          '    }\n' +
          '  }\n' +
          '  module.exports = { config, schema, transform, Service, handlers: [handler_0, handler_1, handler_2, handler_3, handler_4, handler_5, handler_6, handler_7] };\n' +
          '},\n'
      );
    }
    parts.push('};\n');
    parts.push(
      'var __cache__ = {};\n' +
        'function __require__(id) {\n' +
        '  if (__cache__[id]) return __cache__[id].exports;\n' +
        '  var module = (__cache__[id] = { exports: {} });\n' +
        '  __modules__[id](module, module.exports, __require__);\n' +
        '  return module.exports;\n' +
        '}\n' +
        'var __entry__ = __require__(0);\n' +
        `for (var i = 1; i < ${moduleCount}; i += 97) __require__(i);\n` +
        `module.exports = { entry: __entry__, require: __require__, count: ${moduleCount} };\n`
    );
    parts.push('})();\n');
    fs.writeFileSync(path.join(distDir, name), parts.join(''));
  };

  // Sizes mirror real apps: a main bundle, a larger vendor bundle, a preload.
  makeBundle('main.bundle.js', 2600); // ~8 MB
  makeBundle('vendor.bundle.js', 3900); // ~12 MB
  makeBundle('preload.bundle.js', 650); // ~2 MB

  // --- Unbundled long-tail pattern: a small module dependency tree.
  const libDir = path.join(srcDir, 'lib');
  fs.mkdirSync(libDir, { recursive: true });
  const leaves = 100;
  const midSize = 20;
  for (let i = 0; i < leaves; i++) {
    fs.writeFileSync(
      path.join(libDir, `leaf-${i}.js`),
      "'use strict';\n" +
        `const data = { id: ${i}, values: [${Array.from({ length: 30 }, (_, j) => i * j).join(',')}] };\n` +
        'function compute(x) { let acc = 0; for (const v of data.values) acc += v * x; return acc; }\n' +
        'module.exports = { data, compute };\n'
    );
  }
  const mids = Math.floor(leaves / midSize);
  for (let m = 0; m < mids; m++) {
    const requires = Array.from({ length: midSize }, (_, j) => `require('./leaf-${m * midSize + j}.js')`).join(', ');
    fs.writeFileSync(
      path.join(libDir, `mid-${m}.js`),
      `'use strict';\nconst leaves = [${requires}];\n` +
        'module.exports = { leaves, sum: () => leaves.reduce((a, l) => a + l.compute(2), 0) };\n'
    );
  }
  const midRequires = Array.from({ length: mids }, (_, m) => `require('./lib/mid-${m}.js')`).join(', ');
  fs.writeFileSync(
    path.join(srcDir, 'index.js'),
    `'use strict';\nconst mids = [${midRequires}];\n` +
      'module.exports = { mids, total: () => mids.reduce((a, m) => a + m.sum(), 0) };\n'
  );

  // --- Resources: locales, pages, manifest (read, not required).
  const resourcesDir = path.join(srcDir, 'resources');
  fs.mkdirSync(resourcesDir, { recursive: true });
  for (let i = 0; i < 20; i++) {
    const messages = {};
    for (let k = 0; k < 200; k++) messages[`string_${k}`] = `Translated message number ${k} for locale ${i}`;
    fs.writeFileSync(path.join(resourcesDir, `locale-${i}.json`), JSON.stringify(messages));
  }
  for (let i = 0; i < 10; i++) {
    fs.writeFileSync(
      path.join(resourcesDir, `page-${i}.html`),
      `<!DOCTYPE html><html><head><title>Page ${i}</title></head><body>` +
        Array.from({ length: 50 }, (_, j) => `<div class="row" data-id="${j}">Content row ${j}</div>`).join('') +
        '</body></html>'
    );
  }
  fs.writeFileSync(
    path.join(srcDir, 'package.json'),
    JSON.stringify({ name: 'pgo-fixture-app', version: '1.0.0', main: 'dist/main.bundle.js' }, null, 2)
  );

  await asar.createPackage(srcDir, archivePath);
  log(`packaged-app fixture: ${archivePath} (${(fs.statSync(archivePath).size / 1024 / 1024).toFixed(1)} MB)`);
  return archivePath;
}

function ensureLlvmProfdata() {
  const exeSuffix = process.platform === 'win32' ? '.exe' : '';
  const coverageToolsDir = path.join(SRC_DIR, 'third_party', 'llvm-coverage-tools');
  const profdataPath = path.join(coverageToolsDir, 'bin', `llvm-profdata${exeSuffix}`);
  if (fs.existsSync(profdataPath)) return profdataPath;

  log('downloading llvm coverage tools (llvm-profdata)');
  execFileSync(
    'python3',
    [
      path.join(SRC_DIR, 'tools', 'clang', 'scripts', 'update.py'),
      '--package=coverage_tools',
      `--output-dir=${coverageToolsDir}`
    ],
    { stdio: 'inherit' }
  );

  if (!fs.existsSync(profdataPath)) {
    throw new Error(`llvm-profdata not found at ${profdataPath} after download`);
  }
  return profdataPath;
}

async function main() {
  const args = parseArgs(process.argv);
  if (!args.electron || !args.output) {
    console.error(
      'Usage: collect-profile.js --electron <binary> --output <profdata> [--work-dir <dir>] [--port <port>]'
    );
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
  fs.mkdirSync(benchmarksDir, { recursive: true });

  // 1. Benchmarks, TLS material, and the local server.
  //
  //    The server prefers HTTPS + HTTP/2 with a CA installed into the host
  //    trust store, so resource loads and the network workloads train
  //    Chromium's TLS handshake, certificate verification, and H2 session
  //    code. If cert generation or trust installation fails, collection falls
  //    back to plain HTTP (lower network coverage, everything else intact).
  fetchBenchmarks(benchmarksDir);
  const asarFixture = await generateAsarFixture(workDir);
  let tls = null;
  let caCertPath = null;
  const certs = generateCerts(path.join(workDir, 'certs'));
  if (certs && installTrustedCA(certs.caCertPath)) {
    tls = { certPath: certs.certPath, keyPath: certs.keyPath };
    caCertPath = certs.caCertPath;
  }
  // Cap concurrent HTTP/2 streams on every host. Benchmark pages (JetStream
  // especially) fire hundreds of resource fetches at once; fully multiplexed
  // over one connection, the concurrent TLS and stream buffers overwhelm
  // slower collection hosts - 32-bit Windows exhausts its address space, and
  // 4-core arm64 / Windows runners hit page-load errors in JetStream's
  // driver. Capping also keeps serving realistic: no production server
  // grants a single connection unbounded concurrency. Verified by run #15:
  // capped win-x86 collected successfully on the same runner type where
  // uncapped win-x64 failed.
  const server = startServer(benchmarksDir, port, { tls, maxConcurrentStreams: 8 });
  const baseUrl = tls ? `https://localhost:${port}` : `http://127.0.0.1:${port}`;

  // 2. Run the instrumented build through the workloads.
  //    %m = profile merge pools: multiple processes share pool files and merge
  //    counters atomically, keeping the file count manageable.
  // %c (continuous mode, Darwin): counters live in an mmap established at
  // process startup, before the renderer sandbox locks down, so sandboxed
  // child processes don't need an exit-time file write. Without it, renderer
  // profraws race sandbox lockdown and lose ("Operation not permitted") on a
  // per-process coin flip - collections that lose the web-benchmark renderers
  // ship profiles with Blink effectively absent from the hot set.
  // %c must pair with %p (one file per process), NOT %m: the continuous-mode
  // mmap is MAP_SHARED over __llvm_prf_data, whose Values field holds raw
  // heap pointers to value-profiling nodes. With shared pool files, a fresh
  // process reads another process's heap pointer out of the pool and
  // dereferences it in __llvm_profile_instrument_target - an instant SIGSEGV
  // under ASLR that killed renderers on a per-spawn coin flip on mac arm64
  // (where -vp-counters-per-site=6 makes value sites densest).
  const profilePattern = path.join(
    profrawDir,
    process.platform === 'darwin' ? 'electron-%c%p.profraw' : 'electron-%4m.profraw'
  );
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

  // A failed attempt's counters cannot be separated from good ones once they
  // are on disk (and Darwin's %c continuous-mode counters accumulate for the
  // whole app lifetime), so failures are retried by wiping the profraw dir
  // and relaunching the app from scratch: a published profile only ever
  // contains counters from one fully-successful run. Non-final attempts
  // abort on the first workload failure (PGO_ABORT_ON_FAILURE) since their
  // output is discarded anyway; the final attempt runs to completion so a
  // persistent failure still yields a maximal partial profile.
  const maxAttempts = parseInt(args.attempts || '5', 10);
  // Attempts can be slow when a workload burns its own timeout before
  // failing (jetstream2 alone allows 45 minutes), so the attempt count alone
  // does not bound the step duration. Stop launching retries once this much
  // wall clock has elapsed: the in-flight attempt must finish and cleanly
  // flush its counters before the CI step timeout kills the process tree (a
  // hard kill loses the exit-time profraw write entirely on Windows/Linux).
  const retryDeadlineMs = parseInt(args['retry-deadline-minutes'] || '240', 10) * 60 * 1000;
  const collectionStart = Date.now();
  let exitCode = 1;
  let failedWorkloads = [];
  for (let attempt = 1; attempt <= maxAttempts; attempt++) {
    const finalAttempt = attempt === maxAttempts;
    // Always start clean: counters from a failed attempt (or stale %m pool
    // files from a previous run, which append) must not merge into this one.
    fs.rmSync(profrawDir, { recursive: true, force: true });
    fs.mkdirSync(profrawDir, { recursive: true });
    fs.rmSync(resultsFile, { force: true });

    log(`collection attempt ${attempt}/${maxAttempts}`);
    const startTime = Date.now();
    exitCode = await new Promise((resolve, reject) => {
      const child = spawn(electronBinary, electronArgs, {
        env: {
          ...process.env,
          LLVM_PROFILE_FILE: profilePattern,
          PGO_BENCHMARK_BASE_URL: baseUrl,
          PGO_RESULTS_FILE: resultsFile,
          PGO_ASAR_FIXTURE: asarFixture,
          ...(finalAttempt ? {} : { PGO_ABORT_ON_FAILURE: '1' }),
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

    const elapsedMin = ((Date.now() - startTime) / 60000).toFixed(1);
    log(`instrumented run finished with exit code ${exitCode} in ${elapsedMin} minutes`);

    // Electron's clean-shutdown path (app.quit()) always exits 0, so workload
    // failures are reported through the results file rather than the exit code.
    failedWorkloads = [];
    if (fs.existsSync(resultsFile)) {
      const results = JSON.parse(fs.readFileSync(resultsFile, 'utf8'));
      log(`workload results:\n${JSON.stringify(results, null, 2)}`);
      failedWorkloads = results.filter((r) => !r.ok);
    } else {
      log('WARNING: no results file was written - the app may have crashed');
      failedWorkloads = [{ name: 'all', error: 'results file missing' }];
    }
    if (exitCode === 0 && failedWorkloads.length > 0) {
      exitCode = 1;
    }
    if (exitCode === 0 || finalAttempt) break;
    if (Date.now() - collectionStart > retryDeadlineMs) {
      log(`retry deadline reached after attempt ${attempt} - keeping this attempt's partial output`);
      break;
    }
    const reason = failedWorkloads.map((w) => w.name).join(', ') || `exit code ${exitCode}`;
    log(`attempt ${attempt} failed (${reason}) - wiping profiles and retrying`);
  }
  server.close();

  // 3. Merge (or hand off the raw files for later merging).
  const profrawFiles = fs
    .readdirSync(profrawDir)
    .filter((f) => f.endsWith('.profraw'))
    .map((f) => path.join(profrawDir, f));
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
    log(`FAILED workloads: ${failedWorkloads.map((w) => w.name).join(', ')}`);
  }
  process.exit(exitCode);
}

main().catch((err) => {
  console.error('[collect-profile] FATAL:', err);
  process.exit(1);
});
