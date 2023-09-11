import { expect } from 'chai';
import * as http from 'node:http';
import * as http2 from 'node:http2';
import * as qs from 'node:querystring';
import * as path from 'node:path';
import * as fs from 'node:fs';
import * as url from 'node:url';
import * as WebSocket from 'ws';
import { ipcMain, protocol, session, WebContents, webContents } from 'electron/main';
import { Socket } from 'node:net';
import { listen, defer } from './lib/spec-helpers';
import { once } from 'node:events';
import { ReadableStream } from 'node:stream/web';

const fixturesPath = path.resolve(__dirname, 'fixtures');

describe('webRequest module', () => {
  const ses = session.defaultSession;
  const server = http.createServer((req, res) => {
    if (req.url === '/serverRedirect') {
      res.statusCode = 301;
      res.setHeader('Location', 'http://' + req.rawHeaders[1]);
      res.end();
    } else if (req.url === '/contentDisposition') {
      res.writeHead(200, [
        'content-disposition',
        Buffer.from('attachment; filename=aa中aa.txt').toString('binary')
      ]);
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
  let http2URL: string;

  const certPath = path.join(fixturesPath, 'certificates');
  const h2server = http2.createSecureServer({
    key: fs.readFileSync(path.join(certPath, 'server.key')),
    cert: fs.readFileSync(path.join(certPath, 'server.pem'))
  }, async (req, res) => {
    if (req.method === 'POST') {
      const chunks = [];
      for await (const chunk of req) chunks.push(chunk);
      res.end(Buffer.concat(chunks).toString('utf8'));
    } else {
      res.end('<html></html>');
    }
  });

  before(async () => {
    protocol.registerStringProtocol('cors', (req, cb) => cb(''));
    defaultURL = (await listen(server)).url + '/';
    http2URL = (await listen(h2server)).url + '/';
    console.log(http2URL);
  });

  after(() => {
    server.close();
    h2server.close();
    protocol.unregisterProtocol('cors');
  });

  let contents: WebContents;
  // NB. sandbox: true is used because it makes navigations much (~8x) faster.
  before(async () => {
    contents = (webContents as typeof ElectronInternal.WebContents).create({ sandbox: true });
    // const w = new BrowserWindow({webPreferences: {sandbox: true}})
    // contents = w.webContents
    await contents.loadFile(path.join(fixturesPath, 'pages', 'fetch.html'));
  });
  after(() => contents.destroy());

  async function ajax (url: string, options = {}) {
    return contents.executeJavaScript(`ajax("${url}", ${JSON.stringify(options)})`);
  }

  describe('webRequest.onBeforeRequest', () => {
    afterEach(() => {
      ses.webRequest.onBeforeRequest(null);
    });

    const cancel = (details: Electron.OnBeforeRequestListenerDetails, callback: (response: Electron.CallbackResponse) => void) => {
      callback({ cancel: true });
    };

    it('can cancel the request', async () => {
      ses.webRequest.onBeforeRequest(cancel);
      await expect(ajax(defaultURL)).to.eventually.be.rejected();
    });

    it('can filter URLs', async () => {
      const filter = { urls: [defaultURL + 'filter/*'] };
      ses.webRequest.onBeforeRequest(filter, cancel);
      const { data } = await ajax(`${defaultURL}nofilter/test`);
      expect(data).to.equal('/nofilter/test');
      await expect(ajax(`${defaultURL}filter/test`)).to.eventually.be.rejected();
    });

    it('can filter URLs and types', async () => {
      const filter1: Electron.WebRequestFilter = { urls: [defaultURL + 'filter/*'], types: ['xhr'] };
      ses.webRequest.onBeforeRequest(filter1, cancel);
      const { data } = await ajax(`${defaultURL}nofilter/test`);
      expect(data).to.equal('/nofilter/test');
      await expect(ajax(`${defaultURL}filter/test`)).to.eventually.be.rejected();

      const filter2: Electron.WebRequestFilter = { urls: [defaultURL + 'filter/*'], types: ['stylesheet'] };
      ses.webRequest.onBeforeRequest(filter2, cancel);
      expect((await ajax(`${defaultURL}nofilter/test`)).data).to.equal('/nofilter/test');
      expect((await ajax(`${defaultURL}filter/test`)).data).to.equal('/filter/test');
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
        pathname: path.join(fixturesPath, 'blank.html').replaceAll('\\', '/'),
        protocol: 'file',
        slashes: true
      });
      await expect(ajax(fileURL)).to.eventually.be.rejected();
    });

    it('can handle a streaming upload', async () => {
      // Streaming fetch uploads are only supported on HTTP/2, which is only
      // supported over TLS, so...
      session.defaultSession.setCertificateVerifyProc((req, cb) => cb(0));
      defer(() => {
        session.defaultSession.setCertificateVerifyProc(null);
      });
      const contents = (webContents as typeof ElectronInternal.WebContents).create({ sandbox: true });
      defer(() => contents.close());
      await contents.loadURL(http2URL);

      ses.webRequest.onBeforeRequest((details, callback) => {
        callback({});
      });

      const result = await contents.executeJavaScript(`
        const stream = new ReadableStream({
          async start(controller) {
            controller.enqueue('hello world');
            controller.close();
          },
        }).pipeThrough(new TextEncoderStream());
        fetch("${http2URL}", {
          method: 'POST',
          body: stream,
          duplex: 'half',
        }).then(r => r.text())
      `);
      expect(result).to.equal('hello world');
    });

    it('can handle a streaming upload if the uploadData is read', async () => {
      // Streaming fetch uploads are only supported on HTTP/2, which is only
      // supported over TLS, so...
      session.defaultSession.setCertificateVerifyProc((req, cb) => cb(0));
      defer(() => {
        session.defaultSession.setCertificateVerifyProc(null);
      });
      const contents = (webContents as typeof ElectronInternal.WebContents).create({ sandbox: true });
      defer(() => contents.close());
      await contents.loadURL(http2URL);

      function makeStreamFromPipe (pipe: any): ReadableStream {
        const buf = new Uint8Array(1024 * 1024 /* 1 MB */);
        return new ReadableStream({
          async pull (controller) {
            try {
              const rv = await pipe.read(buf);
              if (rv > 0) {
                controller.enqueue(buf.subarray(0, rv));
              } else {
                controller.close();
              }
            } catch (e) {
              controller.error(e);
            }
          }
        });
      }

      ses.webRequest.onBeforeRequest(async (details, callback) => {
        const chunks = [];
        for await (const chunk of makeStreamFromPipe((details.uploadData[0] as any).body)) { chunks.push(chunk); }
        callback({});
      });

      const result = await contents.executeJavaScript(`
        const stream = new ReadableStream({
          async start(controller) {
            controller.enqueue('hello world');
            controller.close();
          },
        }).pipeThrough(new TextEncoderStream());
        fetch("${http2URL}", {
          method: 'POST',
          body: stream,
          duplex: 'half',
        }).then(r => r.text())
      `);

      // NOTE: since the upload stream was consumed by the onBeforeRequest
      // handler, it can't be used again to upload to the actual server.
      // This is a limitation of the WebRequest API.
      expect(result).to.equal('');
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
        pathname: path.join(fixturesPath, 'blank.html').replaceAll('\\', '/'),
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
      const disposition = Buffer.from('attachment; filename=aa中aa.txt').toString('binary');
      expect(headers).to.to.have.property('content-disposition', disposition);
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
      server.on('upgrade', function upgrade (request, socket, head) {
        const pathname = require('node:url').parse(request.url).pathname;
        if (pathname === '/websocket') {
          reqHeaders[request.url!] = request.headers;
          wss.handleUpgrade(request, socket as Socket, head, function done (ws) {
            wss.emit('connection', ws, request);
          });
        }
      });

      // Start server.
      const { port } = await listen(server);

      // Use a separate session for testing.
      const ses = session.fromPartition('WebRequestWebSocket');

      // Setup listeners.
      const receivedHeaders : { [key: string] : any } = {};
      ses.webRequest.onBeforeSendHeaders((details, callback) => {
        details.requestHeaders.foo = 'bar';
        callback({ requestHeaders: details.requestHeaders });
      });
      ses.webRequest.onHeadersReceived((details, callback) => {
        const pathname = require('node:url').parse(details.url).pathname;
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

      const contents = (webContents as typeof ElectronInternal.WebContents).create({
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

      contents.loadFile(path.join(fixturesPath, 'api', 'webrequest.html'), { query: { port: `${port}` } });
      await once(ipcMain, 'websocket-success');

      expect(receivedHeaders['/websocket'].Upgrade[0]).to.equal('websocket');
      expect(receivedHeaders['/'].foo1[0]).to.equal('bar1');
      expect(reqHeaders['/websocket'].foo).to.equal('bar');
      expect(reqHeaders['/'].foo).to.equal('bar');
    });
  });
});
