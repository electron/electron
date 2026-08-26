// Serves any path with a small body from a separate process, so a spec can
// keep the main process busy without also stalling the server.
const http = require('node:http');

const server = http.createServer((req, res) => {
  if (req.url.startsWith('/redirect')) {
    res.writeHead(301, { Location: '/landed' });
    res.end();
    return;
  }
  res.setHeader('Access-Control-Allow-Origin', '*');
  res.end(req.url);
});
server.listen(0, '127.0.0.1', () => {
  process.send({ url: `http://127.0.0.1:${server.address().port}` });
});
