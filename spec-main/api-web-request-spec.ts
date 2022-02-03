import { expect } from 'chai';
import { Socket, AddressInfo } from 'net';
import * as http from 'http';
import * as qs from 'querystring';
import * as path from 'path';
import * as url from 'url';
import * as WebSocket from 'ws';
import { ipcMain, protocol, session, WebContents, webContents } from 'electron/main';

import { emittedOnce } from './events-helpers';

const fixturesPath = path.resolve(__dirname, 'fixtures');

describe('webRequest module', () => {
  const ses = session.defaultSession;
  const server = http.createServer((req, res) => {
    if (req.url === '/serverRedirect') {
      res.statusCode = 301;
      res.setHeader('Location', 'http://' + req.rawHeaders[1]);
      res.end();
    } else if (req.url === '/contentDisposition') {
      res.setHeader('content-disposition', [' attachment; filename=aa%E4%B8%ADaa.txt']);
      const content = req.url;
      res.end(content);
    } else {
      res.setHeader('Custom', ['Header']);
      let content = req.url;
      if (req.headers.accept === '*/*;test/header') {
        content += 'header/received';
      }
      if (req.headers.origin === 'http://new-origin') {
        content += 'new/origin';
      }
      res.end(content);
    }
  });
  let defaultURL: string;

  before((done) => {
    protocol.registerStringProtocol('cors', (req, cb) => cb(''));
    server.listen(0, '127.0.0.1', () => {
      const port = (server.address() as AddressInfo).port;
      defaultURL = `http://127.0.0.1:${port}/`;
      done();
    });
  });

  after(() => {
    server.close();
    protocol.unregisterProtocol('cors');
  });

  let contents: WebContents = null as unknown as WebContents;
  // NB. sandbox: true is used because it makes navigations much (~8x) faster.
  before(async () => {
    contents = (webContents as any).create({ sandbox: true });
    await contents.loadFile(path.join(fixturesPath, 'pages', 'fetch.html'));
  });
  after(() => (contents as any).destroy());

  async function ajax (url: string, options = {}) {
    return contents.executeJavaScript(`ajax("${url}", ${JSON.stringify(options)})`);
  }

  describe('webRequest.onBeforeRequest', () => {
    afterEach(() => {
      ses.webRequest.onBeforeRequest(null);
    });

    it('can cancel the request', async () => {
      ses.webRequest.onBeforeRequest((details, callback) => {
        callback({
          cancel: true
        });
      });
      await expect(ajax(defaultURL)).to.eventually.be.rejected();
    });

    it('can filter URLs', async () => {
      const filter = { urls: [defaultURL + 'filter/*'] };
      ses.webRequest.onBeforeRequest(filter, (details, callback) => {
        callback({ cancel: true });
      });
      const { data } = await ajax(`${defaultURL}nofilter/test`);
      expect(data).to.equal('/nofilter/test');
      await expect(ajax(`${defaultURL}filter/test`)).to.eventually.be.rejected();
    });

    it('receives details object', async () => {
      ses.webRequest.onBeforeRequest((details, callback) => {
        expect(details.id).to.be.a('number');
        expect(details.timestamp).to.be.a('number');
        expect(details.webContentsId).to.be.a('number');
        expect(details.webContents).to.be.an('object');
        expect(details.webContents!.id).to.equal(details.webContentsId);
        expect(details.frame).to.be.an('object');
        expect(details.url).to.be.a('string').that.is.equal(defaultURL);
        expect(details.method).to.be.a('string').that.is.equal('GET');
        expect(details.resourceType).to.be.a('string').that.is.equal('xhr');
        expect(details.uploadData).to.be.undefined();
        callback({});
      });
      const { data } = await ajax(defaultURL);
      expect(data).to.equal('/');
    });

    it('receives post data in details object', async () => {
      const postData = {
        name: 'post test',
        type: 'string'
      };
      ses.webRequest.onBeforeRequest((details, callback) => {
        expect(details.url).to.equal(defaultURL);
        expect(details.method).to.equal('POST');
        expect(details.uploadData).to.have.lengthOf(1);
        const data = qs.parse(details.uploadData[0].bytes.toString());
        expect(data).to.deep.equal(postData);
        callback({ cancel: true });
      });
      await expect(ajax(defaultURL, {
        method: 'POST',
        body: qs.stringify(postData)
      })).to.eventually.be.rejected();
    });

    it('can redirect the request', async () => {
      ses.webRequest.onBeforeRequest((details, callback) => {
        if (details.url === defaultURL) {
          callback({ redirectURL: `${defaultURL}redirect` });
        } else {
          callback({});
        }
      });
      const { data } = await ajax(defaultURL);
      expect(data).to.equal('/redirect');
    });

    it('does not crash for redirects', async () => {
      ses.webRequest.onBeforeRequest((details, callback) => {
        callback({ cancel: false });
      });
      await ajax(defaultURL + 'serverRedirect');
      await ajax(defaultURL + 'serverRedirect');
    });

    it('works with file:// protocol', async () => {
      ses.webRequest.onBeforeRequest((details, callback) => {
        callback({ cancel: true });
      });
      const fileURL = url.format({
        pathname: path.join(fixturesPath, 'blank.html').replace(/\\/g, '/'),
        protocol: 'file',
        slashes: true
      });
      await expect(ajax(fileURL)).to.eventually.be.rejected();
    });
  });

  describe('webRequest.onBeforeSendHeaders', () => {
    afterEach(() => {
      ses.webRequest.onBeforeSendHeaders(null);
      ses.webRequest.onSendHeaders(null);
    });

    it('receives details object', async () => {
      ses.webRequest.onBeforeSendHeaders((details, callback) => {
        expect(details.requestHeaders).to.be.an('object');
        expect(details.requestHeaders['Foo.Bar']).to.equal('baz');
        callback({});
      });
      const { data } = await ajax(defaultURL, { headers: { 'Foo.Bar': 'baz' } });
      expect(data).to.equal('/');
    });

    it('can change the request headers', async () => {
      ses.webRequest.onBeforeSendHeaders((details, callback) => {
        const requestHeaders = details.requestHeaders;
        requestHeaders.Accept = '*/*;test/header';
        callback({ requestHeaders: requestHeaders });
      });
      const { data } = await ajax(defaultURL);
      expect(data).to.equal('/header/received');
    });

    it('can change the request headers on a custom protocol redirect', async () => {
      protocol.registerStringProtocol('no-cors', (req, callback) => {
        if (req.url === 'no-cors://fake-host/redirect') {
          callback({
            statusCode: 302,
            headers: {
              Location: 'no-cors://fake-host'
            }
          });
        } else {
          let content = '';
          if (req.headers.Accept === '*/*;test/header') {
            content = 'header-received';
          }
          callback(content);
        }
      });

      // Note that we need to do navigation every time after a protocol is
      // registered or unregistered, otherwise the new protocol won't be
      // recognized by current page when NetworkService is used.
      await contents.loadFile(path.join(__dirname, 'fixtures', 'pages', 'fetch.html'));

      try {
        ses.webRequest.onBeforeSendHeaders((details, callback) => {
          const requestHeaders = details.requestHeaders;
          requestHeaders.Accept = '*/*;test/header';
          callback({ requestHeaders: requestHeaders });
        });
        const { data } = await ajax('no-cors://fake-host/redirect');
        expect(data).to.equal('header-received');
      } finally {
        protocol.unregisterProtocol('no-cors');
      }
    });

    it('can change request origin', async () => {
      ses.webRequest.onBeforeSendHeaders((details, callback) => {
        const requestHeaders = details.requestHeaders;
        requestHeaders.Origin = 'http://new-origin';
        callback({ requestHeaders: requestHeaders });
      });
      const { data } = await ajax(defaultURL);
      expect(data).to.equal('/new/origin');
    });

    it('can capture CORS requests', async () => {
      let called = false;
      ses.webRequest.onBeforeSendHeaders((details, callback) => {
        called = true;
        callback({ requestHeaders: details.requestHeaders });
      });
      await ajax('cors://host');
      expect(called).to.be.true();
    });

    it('resets the whole headers', async () => {
      const requestHeaders = {
        Test: 'header'
      };
      ses.webRequest.onBeforeSendHeaders((details, callback) => {
        callback({ requestHeaders: requestHeaders });
      });
      ses.webRequest.onSendHeaders((details) => {
        expect(details.requestHeaders).to.deep.equal(requestHeaders);
      });
      await ajax(defaultURL);
    });

    it('leaves headers unchanged when no requestHeaders in callback', async () => {
      let originalRequestHeaders: Record<string, string>;
      ses.webRequest.onBeforeSendHeaders((details, callback) => {
        originalRequestHeaders = details.requestHeaders;
        callback({});
      });
      ses.webRequest.onSendHeaders((details) => {
        expect(details.requestHeaders).to.deep.equal(originalRequestHeaders);
      });
      await ajax(defaultURL);
    });

    it('works with file:// protocol', async () => {
      const requestHeaders = {
        Test: 'header'
      };
      let onSendHeadersCalled = false;
      ses.webRequest.onBeforeSendHeaders((details, callback) => {
        callback({ requestHeaders: requestHeaders });
      });
      ses.webRequest.onSendHeaders((details) => {
        expect(details.requestHeaders).to.deep.equal(requestHeaders);
        onSendHeadersCalled = true;
      });
      await ajax(url.format({
        pathname: path.join(fixturesPath, 'blank.html').replace(/\\/g, '/'),
        protocol: 'file',
        slashes: true
      }));
      expect(onSendHeadersCalled).to.be.true();
    });
  });

  describe('webRequest.onSendHeaders', () => {
    afterEach(() => {
      ses.webRequest.onSendHeaders(null);
    });

    it('receives details object', async () => {
      ses.webRequest.onSendHeaders((details) => {
        expect(details.requestHeaders).to.be.an('object');
      });
      const { data } = await ajax(defaultURL);
      expect(data).to.equal('/');
    });
  });

  describe('webRequest.onHeadersReceived', () => {
    afterEach(() => {
      ses.webRequest.onHeadersReceived(null);
    });

    it('receives details object', async () => {
      ses.webRequest.onHeadersReceived((details, callback) => {
        expect(details.statusLine).to.equal('HTTP/1.1 200 OK');
        expect(details.statusCode).to.equal(200);
        expect(details.responseHeaders!.Custom).to.deep.equal(['Header']);
        callback({});
      });
      const { data } = await ajax(defaultURL);
      expect(data).to.equal('/');
    });

    it('can change the response header', async () => {
      ses.webRequest.onHeadersReceived((details, callback) => {
        const responseHeaders = details.responseHeaders!;
        responseHeaders.Custom = ['Changed'] as any;
        callback({ responseHeaders: responseHeaders });
      });
      const { headers } = await ajax(defaultURL);
      expect(headers).to.to.have.property('custom', 'Changed');
    });

    it('can change response origin', async () => {
      ses.webRequest.onHeadersReceived((details, callback) => {
        const responseHeaders = details.responseHeaders!;
        responseHeaders['access-control-allow-origin'] = ['http://new-origin'] as any;
        callback({ responseHeaders: responseHeaders });
      });
      const { headers } = await ajax(defaultURL);
      expect(headers).to.to.have.property('access-control-allow-origin', 'http://new-origin');
    });

    it('can change headers of CORS responses', async () => {
      ses.webRequest.onHeadersReceived((details, callback) => {
        const responseHeaders = details.responseHeaders!;
        responseHeaders.Custom = ['Changed'] as any;
        callback({ responseHeaders: responseHeaders });
      });
      const { headers } = await ajax('cors://host');
      expect(headers).to.to.have.property('custom', 'Changed');
    });

    it('does not change header by default', async () => {
      ses.webRequest.onHeadersReceived((details, callback) => {
        callback({});
      });
      const { data, headers } = await ajax(defaultURL);
      expect(headers).to.to.have.property('custom', 'Header');
      expect(data).to.equal('/');
    });

    it('does not change content-disposition header by default', async () => {
      ses.webRequest.onHeadersReceived((details, callback) => {
        expect(details.responseHeaders!['content-disposition']).to.deep.equal([' attachment; filename="aa中aa.txt"']);
        callback({});
      });
      const { data, headers } = await ajax(defaultURL + 'contentDisposition');
      expect(headers).to.to.have.property('content-disposition', 'attachment; filename=aa%E4%B8%ADaa.txt');
      expect(data).to.equal('/contentDisposition');
    });

    it('follows server redirect', async () => {
      ses.webRequest.onHeadersReceived((details, callback) => {
        const responseHeaders = details.responseHeaders;
        callback({ responseHeaders: responseHeaders });
      });
      const { headers } = await ajax(defaultURL + 'serverRedirect');
      expect(headers).to.to.have.property('custom', 'Header');
    });

    it('can change the header status', async () => {
      ses.webRequest.onHeadersReceived((details, callback) => {
        const responseHeaders = details.responseHeaders;
        callback({
          responseHeaders: responseHeaders,
          statusLine: 'HTTP/1.1 404 Not Found'
        });
      });
      const { headers } = await ajax(defaultURL);
      expect(headers).to.to.have.property('custom', 'Header');
    });
  });

  describe('webRequest.onResponseStarted', () => {
    afterEach(() => {
      ses.webRequest.onResponseStarted(null);
    });

    it('receives details object', async () => {
      ses.webRequest.onResponseStarted((details) => {
        expect(details.fromCache).to.be.a('boolean');
        expect(details.statusLine).to.equal('HTTP/1.1 200 OK');
        expect(details.statusCode).to.equal(200);
        expect(details.responseHeaders!.Custom).to.deep.equal(['Header']);
      });
      const { data, headers } = await ajax(defaultURL);
      expect(headers).to.to.have.property('custom', 'Header');
      expect(data).to.equal('/');
    });
  });

  describe('webRequest.onBeforeRedirect', () => {
    afterEach(() => {
      ses.webRequest.onBeforeRedirect(null);
      ses.webRequest.onBeforeRequest(null);
    });

    it('receives details object', async () => {
      const redirectURL = defaultURL + 'redirect';
      ses.webRequest.onBeforeRequest((details, callback) => {
        if (details.url === defaultURL) {
          callback({ redirectURL: redirectURL });
        } else {
          callback({});
        }
      });
      ses.webRequest.onBeforeRedirect((details) => {
        expect(details.fromCache).to.be.a('boolean');
        expect(details.statusLine).to.equal('HTTP/1.1 307 Internal Redirect');
        expect(details.statusCode).to.equal(307);
        expect(details.redirectURL).to.equal(redirectURL);
      });
      const { data } = await ajax(defaultURL);
      expect(data).to.equal('/redirect');
    });
  });

  describe('webRequest.onCompleted', () => {
    afterEach(() => {
      ses.webRequest.onCompleted(null);
    });

    it('receives details object', async () => {
      ses.webRequest.onCompleted((details) => {
        expect(details.fromCache).to.be.a('boolean');
        expect(details.statusLine).to.equal('HTTP/1.1 200 OK');
        expect(details.statusCode).to.equal(200);
      });
      const { data } = await ajax(defaultURL);
      expect(data).to.equal('/');
    });
  });

  describe('webRequest.onErrorOccurred', () => {
    afterEach(() => {
      ses.webRequest.onErrorOccurred(null);
      ses.webRequest.onBeforeRequest(null);
    });

    it('receives details object', async () => {
      ses.webRequest.onBeforeRequest((details, callback) => {
        callback({ cancel: true });
      });
      ses.webRequest.onErrorOccurred((details) => {
        expect(details.error).to.equal('net::ERR_BLOCKED_BY_CLIENT');
      });
      await expect(ajax(defaultURL)).to.eventually.be.rejected();
    });
  });

  describe('WebSocket connections', () => {
    it('can be proxyed', async () => {
      // Setup server.
      const reqHeaders : { [key: string] : any } = {};
      const server = http.createServer((req, res) => {
        reqHeaders[req.url!] = req.headers;
        res.setHeader('foo1', 'bar1');
        res.end('ok');
      });
      const wss = new WebSocket.Server({ noServer: true });
      wss.on('connection', function connection (ws) {
        ws.on('message', function incoming (message) {
          if (message === 'foo') {
            ws.send('bar');
          }
        });
      });
      server.on('upgrade', function upgrade (request, socket: Socket, head) {
        const pathname = require('url').parse(request.url).pathname;
        if (pathname === '/websocket') {
          reqHeaders[request.url!] = request.headers;
          wss.handleUpgrade(request, socket, head, function done (ws) {
            wss.emit('connection', ws, request);
          });
        }
      });

      // Start server.
      await new Promise<void>(resolve => server.listen(0, '127.0.0.1', resolve));
      const port = String((server.address() as AddressInfo).port);

      // Use a separate session for testing.
      const ses = session.fromPartition('WebRequestWebSocket');

      // Setup listeners.
      const receivedHeaders : { [key: string] : any } = {};
      ses.webRequest.onBeforeSendHeaders((details, callback) => {
        details.requestHeaders.foo = 'bar';
        callback({ requestHeaders: details.requestHeaders });
      });
      ses.webRequest.onHeadersReceived((details, callback) => {
        const pathname = require('url').parse(details.url).pathname;
        receivedHeaders[pathname] = details.responseHeaders;
        callback({ cancel: false });
      });
      ses.webRequest.onResponseStarted((details) => {
        if (details.url.startsWith('ws://')) {
          expect(details.responseHeaders!.Connection[0]).be.equal('Upgrade');
        } else if (details.url.startsWith('http')) {
          expect(details.responseHeaders!.foo1[0]).be.equal('bar1');
        }
      });
      ses.webRequest.onSendHeaders((details) => {
        if (details.url.startsWith('ws://')) {
          expect(details.requestHeaders.foo).be.equal('bar');
          expect(details.requestHeaders.Upgrade).be.equal('websocket');
        } else if (details.url.startsWith('http')) {
          expect(details.requestHeaders.foo).be.equal('bar');
        }
      });
      ses.webRequest.onCompleted((details) => {
        if (details.url.startsWith('ws://')) {
          expect(details.error).be.equal('net::ERR_WS_UPGRADE');
        } else if (details.url.startsWith('http')) {
          expect(details.error).be.equal('net::OK');
        }
      });

      const contents = (webContents as any).create({
        session: ses,
        nodeIntegration: true,
        webSecurity: false,
        contextIsolation: false
      });

      // Cleanup.
      after(() => {
        contents.destroy();
        server.close();
        ses.webRequest.onBeforeRequest(null);
        ses.webRequest.onBeforeSendHeaders(null);
        ses.webRequest.onHeadersReceived(null);
        ses.webRequest.onResponseStarted(null);
        ses.webRequest.onSendHeaders(null);
        ses.webRequest.onCompleted(null);
      });

      contents.loadFile(path.join(fixturesPath, 'api', 'webrequest.html'), { query: { port } });
      await emittedOnce(ipcMain, 'websocket-success');

      expect(receivedHeaders['/websocket'].Upgrade[0]).to.equal('websocket');
      expect(receivedHeaders['/'].foo1[0]).to.equal('bar1');
      expect(reqHeaders['/websocket'].foo).to.equal('bar');
      expect(reqHeaders['/'].foo).to.equal('bar');
    });
  });
});
