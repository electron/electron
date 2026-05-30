// Fetches (if needed) and serves the benchmark workloads used for PGO profile
// collection. The benchmark set and the serving model (local static server, no
// external network during collection) mirror Chromium's PGO recipe.
//
// Usage:
//   node script/pgo/serve-benchmarks.js --dir <work-dir> [--port 8765]
//
// Serves:
//   /speedometer/  -> WebKit Speedometer (release/3.1)
//   /jetstream/    -> WebKit JetStream (JetStream2.2)
//   /motionmark/   -> WebKit MotionMark
const { execFileSync } = require('node:child_process');
const fs = require('node:fs');
const http = require('node:http');
const path = require('node:path');

// Pin exact revisions for reproducible profiles.
const BENCHMARKS = [
  {
    name: 'speedometer',
    repo: 'https://github.com/WebKit/Speedometer.git',
    revision: '1386415be8fef2f6b6bbdbe1828872471c5d802a' // release/3.1
  },
  {
    name: 'jetstream',
    repo: 'https://github.com/WebKit/JetStream.git',
    revision: '332d8ee1e4d5c40d9492c56f6865e64a9525ee9f' // JetStream2.2
  },
  {
    name: 'motionmark',
    repo: 'https://github.com/WebKit/MotionMark.git',
    revision: 'fdc371164bed6c825516040cb9e978de8d67ba55'
  }
];

const MIME_TYPES = {
  '.html': 'text/html',
  '.js': 'text/javascript',
  '.mjs': 'text/javascript',
  '.css': 'text/css',
  '.json': 'application/json',
  '.png': 'image/png',
  '.jpg': 'image/jpeg',
  '.svg': 'image/svg+xml',
  '.wasm': 'application/wasm',
  '.woff': 'font/woff',
  '.woff2': 'font/woff2'
};

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

function fetchBenchmarks (workDir) {
  for (const benchmark of BENCHMARKS) {
    const dest = path.join(workDir, benchmark.name);
    if (fs.existsSync(path.join(dest, '.git'))) {
      console.log(`[serve-benchmarks] ${benchmark.name} already present`);
      continue;
    }
    console.log(`[serve-benchmarks] fetching ${benchmark.name} @ ${benchmark.revision}`);
    fs.mkdirSync(dest, { recursive: true });
    execFileSync('git', ['init', '--quiet'], { cwd: dest });
    execFileSync('git', ['remote', 'add', 'origin', benchmark.repo], { cwd: dest });
    execFileSync('git', ['fetch', '--quiet', '--depth', '1', 'origin', benchmark.revision], {
      cwd: dest,
      stdio: 'inherit'
    });
    execFileSync('git', ['checkout', '--quiet', 'FETCH_HEAD'], { cwd: dest });
  }
}

function startServer (workDir, port) {
  const realBase = fs.realpathSync(workDir);
  const server = http.createServer((req, res) => {
    // Map /name/rest -> workDir/name/rest, with directory index handling.
    // Resolve the real path (following symlinks) before the containment check
    // so neither ../ sequences nor symlinks can escape the served directory.
    const urlPath = decodeURIComponent(new URL(req.url, `http://127.0.0.1:${port}`).pathname);
    let filePath;
    try {
      filePath = fs.realpathSync(path.join(workDir, urlPath));
    } catch {
      res.writeHead(404);
      return res.end('not found');
    }
    if (filePath !== realBase && !filePath.startsWith(realBase + path.sep)) {
      res.writeHead(403);
      return res.end('forbidden');
    }
    try {
      if (fs.statSync(filePath).isDirectory()) {
        filePath = path.join(filePath, 'index.html');
      }
      const data = fs.readFileSync(filePath);
      res.writeHead(200, {
        'Content-Type': MIME_TYPES[path.extname(filePath)] || 'application/octet-stream',
        'Cache-Control': 'no-store'
      });
      res.end(data);
    } catch {
      res.writeHead(404);
      res.end('not found');
    }
  });
  server.listen(port, '127.0.0.1', () => {
    console.log(`[serve-benchmarks] serving ${workDir} at http://127.0.0.1:${port}`);
  });
  return server;
}

if (require.main === module) {
  const args = parseArgs(process.argv);
  if (!args.dir) {
    console.error('Usage: node serve-benchmarks.js --dir <work-dir> [--port 8765]');
    process.exit(1);
  }
  const workDir = path.resolve(args.dir);
  fs.mkdirSync(workDir, { recursive: true });
  fetchBenchmarks(workDir);
  startServer(workDir, parseInt(args.port || '8765', 10));
}

module.exports = { fetchBenchmarks, startServer, BENCHMARKS };
