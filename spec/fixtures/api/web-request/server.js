// Serves any path with a small body from a separate process, so a spec can
// keep the main process busy without also stalling the server.
const http = require('node:http');

// /r/<token>/<leg> records the headers it was sent; /log/<token> returns them.
const received = new Map();

const server = http.createServer((req, res) => {
  res.setHeader('Access-Control-Allow-Origin', '*');
  const url = new URL(req.url, 'http://host');
  if (url.pathname === '/hop' || url.pathname === '/slow-hop') {
    const go = () => {
      res.writeHead(302, { Location: url.searchParams.get('to') });
      res.end();
    };
    if (url.pathname === '/hop') {
      go();
    } else {
      const timer = setTimeout(go, 400);
      res.on('close', () => clearTimeout(timer));
    }
    return;
  }
  if (url.pathname.startsWith('/r/')) {
    const [, , token, leg] = url.pathname.split('/');
    const legs = received.get(token) || [];
    legs.push({
      leg,
      host: req.headers.host.split(':')[0],
      token: req.headers['x-token'] ?? null,
      extra: req.headers['x-extra'] ?? null
    });
    received.set(token, legs);
    if (url.searchParams.get('to')) {
      res.writeHead(302, { Location: url.searchParams.get('to') });
      res.end();
    } else {
      res.setHeader('X-Original', 'yes');
      res.end(token);
    }
    return;
  }
  if (url.pathname.startsWith('/log/')) {
    res.setHeader('Content-Type', 'application/json');
    res.end(JSON.stringify(received.get(url.pathname.split('/')[2]) || []));
    return;
  }
  if (req.url === '/slow') {
    res.setHeader('Access-Control-Allow-Origin', '*');
    const timer = setTimeout(() => res.end(req.url), 1000);
    res.on('close', () => clearTimeout(timer));
    return;
  }
  if (req.url === '/page') {
    res.end('<!doctype html><title>webRequest observer test</title>');
    return;
  }
  if (req.url === '/redirect-post') {
    res.writeHead(301, { Location: '/landed-method' });
    res.end();
    return;
  }
  if (req.url.startsWith('/redirect')) {
    res.writeHead(301, { Location: '/landed' });
    res.end();
    return;
  }
  if (req.url === '/landed-method') {
    let bodyLength = 0;
    req.on('data', (chunk) => {
      bodyLength += chunk.length;
    });
    req.on('end', () => {
      res.setHeader('Access-Control-Allow-Origin', '*');
      res.end(`${req.method} ${req.url} ${bodyLength}`);
    });
    return;
  }
  res.setHeader('Access-Control-Allow-Origin', '*');
  res.end(req.url);
});
server.listen(0, '127.0.0.1', () => {
  process.send({ url: `http://127.0.0.1:${server.address().port}` });
});
