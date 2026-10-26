// Fetches (if needed) and serves the benchmark workloads used for PGO profile
// collection. The benchmark set and the serving model (local static server, no
// external network during collection) mirror Chromium's PGO recipe.
//
// Usage:
//   node script/pgo/serve-benchmarks.js --dir <work-dir> [--port 8765] [--tls]
//
// Serves:
//   /speedometer/  -> WebKit Speedometer (release/3.1)
//   /jetstream/    -> WebKit JetStream (JetStream2.2)
//   /motionmark/   -> WebKit MotionMark
//   /__pgo/data?bytes=N   -> N bytes of deterministic payload (network training)
//   /__pgo/echo           -> POST echo (network training)
//   /__pgo/ws             -> WebSocket echo (network training)
//   /__pgo/compressed?bytes=N&encoding=gzip|br|zstd
//                         -> Content-Encoding response (decompression training)
//
// When TLS material is provided the server speaks HTTP/2 with an HTTP/1.1
// fallback, so resource loads during collection train Chromium's TLS
// handshake, certificate verification, and H2 session code.
const { execFileSync } = require('node:child_process');
const crypto = require('node:crypto');
const fs = require('node:fs');
const http = require('node:http');
const http2 = require('node:http2');
const path = require('node:path');
const zlib = require('node:zlib');

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

function fetchBenchmarks(workDir) {
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

// Generates an ephemeral CA plus a leaf certificate for localhost. The CA is
// meant to be installed into the collection host's trust store so that the
// instrumented build exercises the real certificate verification path (a
// bypass flag like --ignore-certificate-errors would train the wrong branch).
// The key material never leaves the collection host and is discarded with it.
function generateCerts(certDir) {
  fs.mkdirSync(certDir, { recursive: true });
  const caKey = path.join(certDir, 'ca.key');
  const caCert = path.join(certDir, 'ca.pem');
  const leafKey = path.join(certDir, 'leaf.key');
  const leafCsr = path.join(certDir, 'leaf.csr');
  const leafCert = path.join(certDir, 'leaf.pem');
  const extFile = path.join(certDir, 'leaf.ext');

  try {
    execFileSync('openssl', ['genrsa', '-out', caKey, '2048'], { stdio: 'pipe' });
    execFileSync(
      'openssl',
      [
        'req',
        '-x509',
        '-new',
        '-key',
        caKey,
        '-sha256',
        '-days',
        '2',
        '-subj',
        '/CN=Electron PGO Ephemeral CA',
        '-out',
        caCert
      ],
      { stdio: 'pipe' }
    );

    execFileSync('openssl', ['genrsa', '-out', leafKey, '2048'], { stdio: 'pipe' });
    execFileSync('openssl', ['req', '-new', '-key', leafKey, '-subj', '/CN=localhost', '-out', leafCsr], {
      stdio: 'pipe'
    });
    fs.writeFileSync(
      extFile,
      [
        'basicConstraints=CA:FALSE',
        'keyUsage=digitalSignature,keyEncipherment',
        'extendedKeyUsage=serverAuth',
        'subjectAltName=DNS:localhost,IP:127.0.0.1'
      ].join('\n')
    );
    execFileSync(
      'openssl',
      [
        'x509',
        '-req',
        '-in',
        leafCsr,
        '-CA',
        caCert,
        '-CAkey',
        caKey,
        '-CAcreateserial',
        '-sha256',
        '-days',
        '2',
        '-extfile',
        extFile,
        '-out',
        leafCert
      ],
      { stdio: 'pipe' }
    );
  } catch (err) {
    console.warn(`[serve-benchmarks] could not generate TLS certificates (${err.message}); falling back to HTTP`);
    return null;
  }

  return { caCertPath: caCert, certPath: leafCert, keyPath: leafKey };
}

// ---------------------------------------------------------------------------
// Request handling (shared between HTTP/1.1 and HTTP/2)
// ---------------------------------------------------------------------------

function handleDynamicRequest(req, res, urlPath, query) {
  if (urlPath === '/__pgo/data') {
    const bytes = Math.min(parseInt(query.get('bytes') || '1024', 10), 64 * 1024 * 1024);
    // Deterministic non-compressible payload so transfer sizes are stable.
    const chunk = crypto.createHash('sha512').update(String(bytes)).digest();
    const payload = Buffer.alloc(bytes);
    for (let off = 0; off < bytes; off += chunk.length) {
      chunk.copy(payload, off, 0, Math.min(chunk.length, bytes - off));
    }
    res.writeHead(200, {
      'Content-Type': 'application/octet-stream',
      'Content-Length': payload.length,
      'Cache-Control': 'no-store',
      'Access-Control-Allow-Origin': '*'
    });
    res.end(payload);
    return true;
  }

  if (urlPath === '/__pgo/echo') {
    const chunks = [];
    req.on('data', (c) => chunks.push(c));
    req.on('end', () => {
      const body = Buffer.concat(chunks);
      res.writeHead(200, {
        'Content-Type': 'application/json',
        'Cache-Control': 'no-store',
        'Access-Control-Allow-Origin': '*'
      });
      res.end(JSON.stringify({ bytes: body.length, sha256: crypto.createHash('sha256').update(body).digest('hex') }));
    });
    return true;
  }

  // Pre-compressed JSON payloads served with a real Content-Encoding header,
  // so the client's network stack (net::GzipSourceStream / BrotliSourceStream /
  // ZstdSourceStream) performs the decompression.
  if (urlPath === '/__pgo/compressed') {
    const bytes = Math.min(parseInt(query.get('bytes') || '102400', 10), 8 * 1024 * 1024);
    const encoding = query.get('encoding') || 'gzip';
    const payload = compressedPayload(bytes, encoding);
    if (!payload) {
      res.writeHead(400);
      return res.end('unsupported encoding');
    }
    const headers = {
      'Content-Type': 'application/json',
      'Cache-Control': 'no-store',
      'Access-Control-Allow-Origin': '*'
    };
    if (encoding !== 'identity') headers['Content-Encoding'] = encoding;
    res.writeHead(200, headers);
    res.end(payload);
    return true;
  }

  return false;
}

// Compressible JSON payloads (rows of records, like an API response),
// compressed once per (size, encoding) and cached.
const compressedCache = new Map();
function compressedPayload(bytes, encoding) {
  const key = `${bytes}:${encoding}`;
  if (compressedCache.has(key)) return compressedCache.get(key);

  const rows = [];
  let size = 0;
  let i = 0;
  while (size < bytes) {
    const row = JSON.stringify({
      id: i,
      name: `record-${i}`,
      email: `user${i}@example.com`,
      tags: ['alpha', 'beta', 'gamma'],
      score: i * 3.14159
    });
    rows.push(row);
    size += row.length;
    i++;
  }
  const raw = Buffer.from('[' + rows.join(',') + ']');

  let out = null;
  if (encoding === 'identity') out = raw;
  else if (encoding === 'gzip') out = zlib.gzipSync(raw);
  else if (encoding === 'br') out = zlib.brotliCompressSync(raw);
  else if (encoding === 'zstd' && zlib.zstdCompressSync) out = zlib.zstdCompressSync(raw);

  compressedCache.set(key, out);
  return out;
}

function makeRequestHandler(workDir, port) {
  const realBase = fs.realpathSync(workDir);
  return (req, res) => {
    // Map /name/rest -> workDir/name/rest, with directory index handling.
    // Resolve the real path (following symlinks) before the containment check
    // so neither ../ sequences nor symlinks can escape the served directory.
    const url = new URL(req.url, `http://127.0.0.1:${port}`);
    const urlPath = decodeURIComponent(url.pathname);

    if (urlPath.startsWith('/__pgo/')) {
      if (handleDynamicRequest(req, res, urlPath, url.searchParams)) return;
      res.writeHead(404);
      return res.end('not found');
    }

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
  };
}

// ---------------------------------------------------------------------------
// Minimal RFC 6455 WebSocket echo (no dependencies). Browsers establish
// WebSocket connections over HTTP/1.1, so this hooks the server's 'upgrade'
// event (which Http2SecureServer also emits for ALPN http/1.1 connections).
// ---------------------------------------------------------------------------

const WS_GUID = '258EAFA5-E914-47DA-95CA-C5AB0DC85B11';

function attachWebSocketEcho(server) {
  server.on('upgrade', (req, socket) => {
    if (!req.url.startsWith('/__pgo/ws')) {
      socket.destroy();
      return;
    }
    const key = req.headers['sec-websocket-key'];
    const accept = crypto
      .createHash('sha1')
      .update(key + WS_GUID)
      .digest('base64');
    socket.write(
      'HTTP/1.1 101 Switching Protocols\r\n' +
        'Upgrade: websocket\r\n' +
        'Connection: Upgrade\r\n' +
        `Sec-WebSocket-Accept: ${accept}\r\n\r\n`
    );

    let buffer = Buffer.alloc(0);
    socket.on('data', (data) => {
      buffer = Buffer.concat([buffer, data]);
      // Parse complete frames; echo data frames back, answer pings, honor close.
      while (buffer.length >= 2) {
        const opcode = buffer[0] & 0x0f;
        const masked = (buffer[1] & 0x80) !== 0;
        let payloadLen = buffer[1] & 0x7f;
        let offset = 2;
        if (payloadLen === 126) {
          if (buffer.length < 4) return;
          payloadLen = buffer.readUInt16BE(2);
          offset = 4;
        } else if (payloadLen === 127) {
          if (buffer.length < 10) return;
          payloadLen = Number(buffer.readBigUInt64BE(2));
          offset = 10;
        }
        const maskLen = masked ? 4 : 0;
        if (buffer.length < offset + maskLen + payloadLen) return;

        let payload = buffer.subarray(offset + maskLen, offset + maskLen + payloadLen);
        if (masked) {
          const mask = buffer.subarray(offset, offset + 4);
          payload = Buffer.from(payload);
          for (let i = 0; i < payload.length; i++) payload[i] ^= mask[i % 4];
        }
        buffer = buffer.subarray(offset + maskLen + payloadLen);

        if (opcode === 0x8) {
          // close
          socket.end(Buffer.from([0x88, 0x00]));
          return;
        }
        const respOpcode = opcode === 0x9 ? 0xa : opcode; // pong for ping, else echo
        // Server-to-client frames are unmasked.
        let header;
        if (payload.length < 126) {
          header = Buffer.from([0x80 | respOpcode, payload.length]);
        } else if (payload.length < 65536) {
          header = Buffer.alloc(4);
          header[0] = 0x80 | respOpcode;
          header[1] = 126;
          header.writeUInt16BE(payload.length, 2);
        } else {
          header = Buffer.alloc(10);
          header[0] = 0x80 | respOpcode;
          header[1] = 127;
          header.writeBigUInt64BE(BigInt(payload.length), 2);
        }
        socket.write(Buffer.concat([header, payload]));
      }
    });
    socket.on('error', () => socket.destroy());
  });
}

// ---------------------------------------------------------------------------
// Server startup
// ---------------------------------------------------------------------------

// opts.tls: { certPath, keyPath } -> serve HTTP/2 + HTTP/1.1 over TLS.
// Without TLS material the server is plain HTTP/1.1 (local development).
function startServer(workDir, port, opts = {}) {
  const handler = makeRequestHandler(workDir, port);
  let server;
  let scheme;

  if (opts.tls) {
    const serverOptions = {
      cert: fs.readFileSync(opts.tls.certPath),
      key: fs.readFileSync(opts.tls.keyPath),
      allowHTTP1: true
    };
    // Cap concurrent HTTP/2 streams when requested. Benchmark pages fire
    // hundreds of resource fetches at once; on constrained clients the
    // multiplexed burst (TLS + stream buffers for every request at once)
    // can exhaust the renderer's address space.
    if (opts.maxConcurrentStreams) {
      serverOptions.settings = {
        maxConcurrentStreams: opts.maxConcurrentStreams
      };
    }
    server = http2.createSecureServer(serverOptions, handler);
    scheme = 'https';
  } else {
    server = http.createServer(handler);
    scheme = 'http';
  }

  // Node closes idle keep-alive sockets after 5s by default. Instrumented
  // Chromium builds intermittently fail to reap a socket the server closed
  // while idle: it stays counted against the per-group connection pool limit
  // (6), and once all six slots are held by dead sockets every later request
  // to the origin queues forever - the next loadURL hangs indefinitely
  // (reproduced locally; the renderer, browser, and network service all sit
  // idle while the navigation waits for a pool slot). Benchmark workloads
  // have idle gaps between phases, so keep sockets alive for far longer than
  // any gap instead. headersTimeout must exceed keepAliveTimeout or node
  // resets sockets mid-request.
  server.keepAliveTimeout = 10 * 60 * 1000;
  server.headersTimeout = 11 * 60 * 1000;

  attachWebSocketEcho(server);

  const host = opts.tls ? 'localhost' : '127.0.0.1';
  server.listen(port, host, () => {
    console.log(`[serve-benchmarks] serving ${workDir} at ${scheme}://${host}:${port}`);
  });
  return server;
}

if (require.main === module) {
  const args = parseArgs(process.argv);
  if (!args.dir) {
    console.error('Usage: node serve-benchmarks.js --dir <work-dir> [--port 8765] [--tls]');
    process.exit(1);
  }
  const workDir = path.resolve(args.dir);
  fs.mkdirSync(workDir, { recursive: true });
  fetchBenchmarks(workDir);
  const port = parseInt(args.port || '8765', 10);
  let tls = null;
  if (args.tls) {
    const certs = generateCerts(path.join(workDir, 'certs'));
    if (certs) tls = { certPath: certs.certPath, keyPath: certs.keyPath };
  }
  startServer(workDir, port, { tls });
}

module.exports = { fetchBenchmarks, startServer, generateCerts, BENCHMARKS };
