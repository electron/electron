import { expect } from 'chai';
import { v4 } from 'uuid';
import { protocol, webContents, WebContents, session, BrowserWindow, ipcMain } from 'electron/main';
import { AddressInfo } from 'net';
import * as ChildProcess from 'child_process';
import * as path from 'path';
import * as http from 'http';
import * as fs from 'fs';
import * as qs from 'querystring';
import * as stream from 'stream';
import { EventEmitter } from 'events';
import { closeWindow } from './window-helpers';
import { emittedOnce } from './events-helpers';
import { WebmGenerator } from './video-helpers';
import { delay } from './spec-helpers';

const fixturesPath = path.resolve(__dirname, '..', 'spec', 'fixtures');

const registerStringProtocol = protocol.registerStringProtocol;
const registerBufferProtocol = protocol.registerBufferProtocol;
const registerFileProtocol = protocol.registerFileProtocol;
const registerHttpProtocol = protocol.registerHttpProtocol;
const registerStreamProtocol = protocol.registerStreamProtocol;
const interceptStringProtocol = protocol.interceptStringProtocol;
const interceptBufferProtocol = protocol.interceptBufferProtocol;
const interceptHttpProtocol = protocol.interceptHttpProtocol;
const interceptStreamProtocol = protocol.interceptStreamProtocol;
const unregisterProtocol = protocol.unregisterProtocol;
const uninterceptProtocol = protocol.uninterceptProtocol;

const text = 'valar morghulis';
const protocolName = 'sp';
const postData = {
  name: 'post test',
  type: 'string'
};

function getStream (chunkSize = text.length, data: Buffer | string = text) {
  const body = new stream.PassThrough();

  async function sendChunks () {
    await delay(0); // the stream protocol API breaks if you send data immediately.
    let buf = Buffer.from(data as any); // nodejs typings are wrong, Buffer.from can take a Buffer
    for (;;) {
      body.push(buf.slice(0, chunkSize));
      buf = buf.slice(chunkSize);
      if (!buf.length) {
        break;
      }
      // emulate some network delay
      await delay(10);
    }
    body.push(null);
  }

  sendChunks();
  return body;
}

// A promise that can be resolved externally.
function defer (): Promise<any> & {resolve: Function, reject: Function} {
  let promiseResolve: Function = null as unknown as Function;
  let promiseReject: Function = null as unknown as Function;
  const promise: any = new Promise((resolve, reject) => {
    promiseResolve = resolve;
    promiseReject = reject;
  });
  promise.resolve = promiseResolve;
  promise.reject = promiseReject;
  return promise;
}

describe('protocol module', () => {
  let contents: WebContents = null as unknown as WebContents;
  // NB. sandbox: true is used because it makes navigations much (~8x) faster.
  before(() => { contents = (webContents as any).create({ sandbox: true }); });
  after(() => (contents as any).destroy());

  async function ajax (url: string, options = {}) {
    // Note that we need to do navigation every time after a protocol is
    // registered or unregistered, otherwise the new protocol won't be
    // recognized by current page when NetworkService is used.
    await contents.loadFile(path.join(__dirname, 'fixtures', 'pages', 'jquery.html'));
    return contents.executeJavaScript(`ajax("${url}", ${JSON.stringify(options)})`);
  }

  afterEach(() => {
    protocol.unregisterProtocol(protocolName);
    protocol.uninterceptProtocol('http');
  });

  describe('protocol.register(Any)Protocol', () => {
    it('fails when scheme is already registered', () => {
      expect(registerStringProtocol(protocolName, (req, cb) => cb(''))).to.equal(true);
      expect(registerBufferProtocol(protocolName, (req, cb) => cb(Buffer.from('')))).to.equal(false);
    });

    it('does not crash when handler is called twice', async () => {
      registerStringProtocol(protocolName, (request, callback) => {
        try {
          callback(text);
          callback('');
        } catch (error) {
          // Ignore error
        }
      });
      const r = await ajax(protocolName + '://fake-host');
      expect(r.data).to.equal(text);
    });

    it('sends error when callback is called with nothing', async () => {
      registerBufferProtocol(protocolName, (req, cb: any) => cb());
      await expect(ajax(protocolName + '://fake-host')).to.eventually.be.rejectedWith(Error, '404');
    });

    it('does not crash when callback is called in next tick', async () => {
      registerStringProtocol(protocolName, (request, callback) => {
        setImmediate(() => callback(text));
      });
      const r = await ajax(protocolName + '://fake-host');
      expect(r.data).to.equal(text);
    });

    it('can redirect to the same scheme', async () => {
      registerStringProtocol(protocolName, (request, callback) => {
        if (request.url === `${protocolName}://fake-host/redirect`) {
          callback({
            statusCode: 302,
            headers: {
              Location: `${protocolName}://fake-host`
            }
          });
        } else {
          expect(request.url).to.equal(`${protocolName}://fake-host`);
          callback('redirected');
        }
      });
      const r = await ajax(`${protocolName}://fake-host/redirect`);
      expect(r.data).to.equal('redirected');
    });
  });

  describe('protocol.unregisterProtocol', () => {
    it('returns false when scheme does not exist', () => {
      expect(unregisterProtocol('not-exist')).to.equal(false);
    });
  });

  describe('protocol.registerStringProtocol', () => {
    it('sends string as response', async () => {
      registerStringProtocol(protocolName, (request, callback) => callback(text));
      const r = await ajax(protocolName + '://fake-host');
      expect(r.data).to.equal(text);
    });

    it('sets Access-Control-Allow-Origin', async () => {
      registerStringProtocol(protocolName, (request, callback) => callback(text));
      const r = await ajax(protocolName + '://fake-host');
      expect(r.data).to.equal(text);
      expect(r.headers).to.include('access-control-allow-origin: *');
    });

    it('sends object as response', async () => {
      registerStringProtocol(protocolName, (request, callback) => {
        callback({
          data: text,
          mimeType: 'text/html'
        });
      });
      const r = await ajax(protocolName + '://fake-host');
      expect(r.data).to.equal(text);
    });

    it('fails when sending object other than string', async () => {
      const notAString = () => {};
      registerStringProtocol(protocolName, (request, callback) => callback(notAString as any));
      await expect(ajax(protocolName + '://fake-host')).to.be.eventually.rejectedWith(Error, '404');
    });
  });

  describe('protocol.registerBufferProtocol', () => {
    const buffer = Buffer.from(text);
    it('sends Buffer as response', async () => {
      registerBufferProtocol(protocolName, (request, callback) => callback(buffer));
      const r = await ajax(protocolName + '://fake-host');
      expect(r.data).to.equal(text);
    });

    it('sets Access-Control-Allow-Origin', async () => {
      registerBufferProtocol(protocolName, (request, callback) => callback(buffer));
      const r = await ajax(protocolName + '://fake-host');
      expect(r.data).to.equal(text);
      expect(r.headers).to.include('access-control-allow-origin: *');
    });

    it('sends object as response', async () => {
      registerBufferProtocol(protocolName, (request, callback) => {
        callback({
          data: buffer,
          mimeType: 'text/html'
        });
      });
      const r = await ajax(protocolName + '://fake-host');
      expect(r.data).to.equal(text);
    });

    it('fails when sending string', async () => {
      registerBufferProtocol(protocolName, (request, callback) => callback(text as any));
      await expect(ajax(protocolName + '://fake-host')).to.be.eventually.rejectedWith(Error, '404');
    });
  });

  describe('protocol.registerFileProtocol', () => {
    const filePath = path.join(fixturesPath, 'test.asar', 'a.asar', 'file1');
    const fileContent = fs.readFileSync(filePath);
    const normalPath = path.join(fixturesPath, 'pages', 'a.html');
    const normalContent = fs.readFileSync(normalPath);

    it('sends file path as response', async () => {
      registerFileProtocol(protocolName, (request, callback) => callback(filePath));
      const r = await ajax(protocolName + '://fake-host');
      expect(r.data).to.equal(String(fileContent));
    });

    it('sets Access-Control-Allow-Origin', async () => {
      registerFileProtocol(protocolName, (request, callback) => callback(filePath));
      const r = await ajax(protocolName + '://fake-host');
      expect(r.data).to.equal(String(fileContent));
      expect(r.headers).to.include('access-control-allow-origin: *');
    });

    it('sets custom headers', async () => {
      registerFileProtocol(protocolName, (request, callback) => callback({
        path: filePath,
        headers: { 'X-Great-Header': 'sogreat' }
      }));
      const r = await ajax(protocolName + '://fake-host');
      expect(r.data).to.equal(String(fileContent));
      expect(r.headers).to.include('x-great-header: sogreat');
    });

    it.skip('throws an error when custom headers are invalid', (done) => {
      registerFileProtocol(protocolName, (request, callback) => {
        expect(() => callback({
          path: filePath,
          headers: { 'X-Great-Header': (42 as any) }
        })).to.throw(Error, 'Value of \'X-Great-Header\' header has to be a string');
        done();
      });
      ajax(protocolName + '://fake-host');
    });

    it('sends object as response', async () => {
      registerFileProtocol(protocolName, (request, callback) => callback({ path: filePath }));
      const r = await ajax(protocolName + '://fake-host');
      expect(r.data).to.equal(String(fileContent));
    });

    it('can send normal file', async () => {
      registerFileProtocol(protocolName, (request, callback) => callback(normalPath));
      const r = await ajax(protocolName + '://fake-host');
      expect(r.data).to.equal(String(normalContent));
    });

    it('fails when sending unexist-file', async () => {
      const fakeFilePath = path.join(fixturesPath, 'test.asar', 'a.asar', 'not-exist');
      registerFileProtocol(protocolName, (request, callback) => callback(fakeFilePath));
      await expect(ajax(protocolName + '://fake-host')).to.be.eventually.rejectedWith(Error, '404');
    });

    it('fails when sending unsupported content', async () => {
      registerFileProtocol(protocolName, (request, callback) => callback(new Date() as any));
      await expect(ajax(protocolName + '://fake-host')).to.be.eventually.rejectedWith(Error, '404');
    });
  });

  describe('protocol.registerHttpProtocol', () => {
    it('sends url as response', async () => {
      const server = http.createServer((req, res) => {
        expect(req.headers.accept).to.not.equal('');
        res.end(text);
        server.close();
      });
      await new Promise<void>(resolve => server.listen(0, '127.0.0.1', resolve));

      const port = (server.address() as AddressInfo).port;
      const url = 'http://127.0.0.1:' + port;
      registerHttpProtocol(protocolName, (request, callback) => callback({ url }));
      const r = await ajax(protocolName + '://fake-host');
      expect(r.data).to.equal(text);
    });

    it('fails when sending invalid url', async () => {
      registerHttpProtocol(protocolName, (request, callback) => callback({ url: 'url' }));
      await expect(ajax(protocolName + '://fake-host')).to.be.eventually.rejectedWith(Error, '404');
    });

    it('fails when sending unsupported content', async () => {
      registerHttpProtocol(protocolName, (request, callback) => callback(new Date() as any));
      await expect(ajax(protocolName + '://fake-host')).to.be.eventually.rejectedWith(Error, '404');
    });

    it('works when target URL redirects', async () => {
      const server = http.createServer((req, res) => {
        if (req.url === '/serverRedirect') {
          res.statusCode = 301;
          res.setHeader('Location', `http://${req.rawHeaders[1]}`);
          res.end();
        } else {
          res.end(text);
        }
      });
      after(() => server.close());
      await new Promise<void>(resolve => server.listen(0, '127.0.0.1', resolve));

      const port = (server.address() as AddressInfo).port;
      const url = `${protocolName}://fake-host`;
      const redirectURL = `http://127.0.0.1:${port}/serverRedirect`;
      registerHttpProtocol(protocolName, (request, callback) => callback({ url: redirectURL }));

      const r = await ajax(url);
      expect(r.data).to.equal(text);
    });

    it('can access request headers', (done) => {
      protocol.registerHttpProtocol(protocolName, (request) => {
        try {
          expect(request).to.have.property('headers');
          done();
        } catch (e) {
          done(e);
        }
      });
      ajax(protocolName + '://fake-host');
    });
  });

  describe('protocol.registerStreamProtocol', () => {
    it('sends Stream as response', async () => {
      registerStreamProtocol(protocolName, (request, callback) => callback(getStream()));
      const r = await ajax(protocolName + '://fake-host');
      expect(r.data).to.equal(text);
    });

    it('sends object as response', async () => {
      registerStreamProtocol(protocolName, (request, callback) => callback({ data: getStream() }));
      const r = await ajax(protocolName + '://fake-host');
      expect(r.data).to.equal(text);
      expect(r.status).to.equal(200);
    });

    it('sends custom response headers', async () => {
      registerStreamProtocol(protocolName, (request, callback) => callback({
        data: getStream(3),
        headers: {
          'x-electron': ['a', 'b']
        }
      }));
      const r = await ajax(protocolName + '://fake-host');
      expect(r.data).to.equal(text);
      expect(r.status).to.equal(200);
      expect(r.headers).to.include('x-electron: a, b');
    });

    it('sends custom status code', async () => {
      registerStreamProtocol(protocolName, (request, callback) => callback({
        statusCode: 204,
        data: null as any
      }));
      const r = await ajax(protocolName + '://fake-host');
      expect(r.data).to.be.undefined('data');
      expect(r.status).to.equal(204);
    });

    it('receives request headers', async () => {
      registerStreamProtocol(protocolName, (request, callback) => {
        callback({
          headers: {
            'content-type': 'application/json'
          },
          data: getStream(5, JSON.stringify(Object.assign({}, request.headers)))
        });
      });
      const r = await ajax(protocolName + '://fake-host', { headers: { 'x-return-headers': 'yes' } });
      expect(r.data['x-return-headers']).to.equal('yes');
    });

    it('returns response multiple response headers with the same name', async () => {
      registerStreamProtocol(protocolName, (request, callback) => {
        callback({
          headers: {
            header1: ['value1', 'value2'],
            header2: 'value3'
          },
          data: getStream()
        });
      });
      const r = await ajax(protocolName + '://fake-host');
      // SUBTLE: when the response headers have multiple values it
      // separates values by ", ". When the response headers are incorrectly
      // converting an array to a string it separates values by ",".
      expect(r.headers).to.equal('header1: value1, value2\r\nheader2: value3\r\n');
    });

    it('can handle large responses', async () => {
      const data = Buffer.alloc(128 * 1024);
      registerStreamProtocol(protocolName, (request, callback) => {
        callback(getStream(data.length, data));
      });
      const r = await ajax(protocolName + '://fake-host');
      expect(r.data).to.have.lengthOf(data.length);
    });

    it('can handle a stream completing while writing', async () => {
      function dumbPassthrough () {
        return new stream.Transform({
          async transform (chunk, encoding, cb) {
            cb(null, chunk);
          }
        });
      }
      registerStreamProtocol(protocolName, (request, callback) => {
        callback({
          statusCode: 200,
          headers: { 'Content-Type': 'text/plain' },
          data: getStream(1024 * 1024, Buffer.alloc(1024 * 1024 * 2)).pipe(dumbPassthrough())
        });
      });
      const r = await ajax(protocolName + '://fake-host');
      expect(r.data).to.have.lengthOf(1024 * 1024 * 2);
    });

    it('can handle next-tick scheduling during read calls', async () => {
      const events = new EventEmitter();
      function createStream () {
        const buffers = [
          Buffer.alloc(65536),
          Buffer.alloc(65537),
          Buffer.alloc(39156)
        ];
        const e = new stream.Readable({ highWaterMark: 0 });
        e.push(buffers.shift());
        e._read = function () {
          process.nextTick(() => this.push(buffers.shift() || null));
        };
        e.on('end', function () {
          events.emit('end');
        });
        return e;
      }
      registerStreamProtocol(protocolName, (request, callback) => {
        callback({
          statusCode: 200,
          headers: { 'Content-Type': 'text/plain' },
          data: createStream()
        });
      });
      const hasEndedPromise = emittedOnce(events, 'end');
      ajax(protocolName + '://fake-host');
      await hasEndedPromise;
    });

    it('destroys response streams when aborted before completion', async () => {
      const events = new EventEmitter();
      registerStreamProtocol(protocolName, (request, callback) => {
        const responseStream = new stream.PassThrough();
        responseStream.push('data\r\n');
        responseStream.on('close', () => {
          events.emit('close');
        });
        callback({
          statusCode: 200,
          headers: { 'Content-Type': 'text/plain' },
          data: responseStream
        });
        events.emit('respond');
      });

      const hasRespondedPromise = emittedOnce(events, 'respond');
      const hasClosedPromise = emittedOnce(events, 'close');
      ajax(protocolName + '://fake-host');
      await hasRespondedPromise;
      await contents.loadFile(path.join(__dirname, 'fixtures', 'pages', 'jquery.html'));
      await hasClosedPromise;
    });
  });

  describe('protocol.isProtocolRegistered', () => {
    it('returns false when scheme is not registered', () => {
      const result = protocol.isProtocolRegistered('no-exist');
      expect(result).to.be.false('no-exist: is handled');
    });

    it('returns true for custom protocol', () => {
      registerStringProtocol(protocolName, (request, callback) => callback(''));
      const result = protocol.isProtocolRegistered(protocolName);
      expect(result).to.be.true('custom protocol is handled');
    });
  });

  describe('protocol.isProtocolIntercepted', () => {
    it('returns true for intercepted protocol', () => {
      interceptStringProtocol('http', (request, callback) => callback(''));
      const result = protocol.isProtocolIntercepted('http');
      expect(result).to.be.true('intercepted protocol is handled');
    });
  });

  describe('protocol.intercept(Any)Protocol', () => {
    it('returns false when scheme is already intercepted', () => {
      expect(protocol.interceptStringProtocol('http', (request, callback) => callback(''))).to.equal(true);
      expect(protocol.interceptBufferProtocol('http', (request, callback) => callback(Buffer.from('')))).to.equal(false);
    });

    it('does not crash when handler is called twice', async () => {
      interceptStringProtocol('http', (request, callback) => {
        try {
          callback(text);
          callback('');
        } catch (error) {
          // Ignore error
        }
      });
      const r = await ajax('http://fake-host');
      expect(r.data).to.be.equal(text);
    });

    it('sends error when callback is called with nothing', async () => {
      interceptStringProtocol('http', (request, callback: any) => callback());
      await expect(ajax('http://fake-host')).to.be.eventually.rejectedWith(Error, '404');
    });
  });

  describe('protocol.interceptStringProtocol', () => {
    it('can intercept http protocol', async () => {
      interceptStringProtocol('http', (request, callback) => callback(text));
      const r = await ajax('http://fake-host');
      expect(r.data).to.equal(text);
    });

    it('can set content-type', async () => {
      interceptStringProtocol('http', (request, callback) => {
        callback({
          mimeType: 'application/json',
          data: '{"value": 1}'
        });
      });
      const r = await ajax('http://fake-host');
      expect(r.data).to.be.an('object');
      expect(r.data).to.have.property('value').that.is.equal(1);
    });

    it('can set content-type with charset', async () => {
      interceptStringProtocol('http', (request, callback) => {
        callback({
          mimeType: 'application/json; charset=UTF-8',
          data: '{"value": 1}'
        });
      });
      const r = await ajax('http://fake-host');
      expect(r.data).to.be.an('object');
      expect(r.data).to.have.property('value').that.is.equal(1);
    });

    it('can receive post data', async () => {
      interceptStringProtocol('http', (request, callback) => {
        const uploadData = request.uploadData![0].bytes.toString();
        callback({ data: uploadData });
      });
      const r = await ajax('http://fake-host', { type: 'POST', data: postData });
      expect({ ...qs.parse(r.data) }).to.deep.equal(postData);
    });
  });

  describe('protocol.interceptBufferProtocol', () => {
    it('can intercept http protocol', async () => {
      interceptBufferProtocol('http', (request, callback) => callback(Buffer.from(text)));
      const r = await ajax('http://fake-host');
      expect(r.data).to.equal(text);
    });

    it('can receive post data', async () => {
      interceptBufferProtocol('http', (request, callback) => {
        const uploadData = request.uploadData![0].bytes;
        callback(uploadData);
      });
      const r = await ajax('http://fake-host', { type: 'POST', data: postData });
      expect(r.data).to.equal('name=post+test&type=string');
    });
  });

  describe('protocol.interceptHttpProtocol', () => {
    // FIXME(zcbenz): This test was passing because the test itself was wrong,
    // I don't know whether it ever passed before and we should take a look at
    // it in future.
    xit('can send POST request', async () => {
      const server = http.createServer((req, res) => {
        let body = '';
        req.on('data', (chunk) => {
          body += chunk;
        });
        req.on('end', () => {
          res.end(body);
        });
        server.close();
      });
      after(() => server.close());
      server.listen(0, '127.0.0.1');

      const port = (server.address() as AddressInfo).port;
      const url = `http://127.0.0.1:${port}`;
      interceptHttpProtocol('http', (request, callback) => {
        const data: Electron.ProtocolResponse = {
          url: url,
          method: 'POST',
          uploadData: {
            contentType: 'application/x-www-form-urlencoded',
            data: request.uploadData![0].bytes
          },
          session: undefined
        };
        callback(data);
      });
      const r = await ajax('http://fake-host', { type: 'POST', data: postData });
      expect({ ...qs.parse(r.data) }).to.deep.equal(postData);
    });

    it('can use custom session', async () => {
      const customSession = session.fromPartition('custom-ses', { cache: false });
      customSession.webRequest.onBeforeRequest((details, callback) => {
        expect(details.url).to.equal('http://fake-host/');
        callback({ cancel: true });
      });
      after(() => customSession.webRequest.onBeforeRequest(null));

      interceptHttpProtocol('http', (request, callback) => {
        callback({
          url: request.url,
          session: customSession
        });
      });
      await expect(ajax('http://fake-host')).to.be.eventually.rejectedWith(Error);
    });

    it('can access request headers', (done) => {
      protocol.interceptHttpProtocol('http', (request) => {
        try {
          expect(request).to.have.property('headers');
          done();
        } catch (e) {
          done(e);
        }
      });
      ajax('http://fake-host');
    });
  });

  describe('protocol.interceptStreamProtocol', () => {
    it('can intercept http protocol', async () => {
      interceptStreamProtocol('http', (request, callback) => callback(getStream()));
      const r = await ajax('http://fake-host');
      expect(r.data).to.equal(text);
    });

    it('can receive post data', async () => {
      interceptStreamProtocol('http', (request, callback) => {
        callback(getStream(3, request.uploadData![0].bytes.toString()));
      });
      const r = await ajax('http://fake-host', { type: 'POST', data: postData });
      expect({ ...qs.parse(r.data) }).to.deep.equal(postData);
    });

    it('can execute redirects', async () => {
      interceptStreamProtocol('http', (request, callback) => {
        if (request.url.indexOf('http://fake-host') === 0) {
          setTimeout(() => {
            callback({
              data: '',
              statusCode: 302,
              headers: {
                Location: 'http://fake-redirect'
              }
            });
          }, 300);
        } else {
          expect(request.url.indexOf('http://fake-redirect')).to.equal(0);
          callback(getStream(1, 'redirect'));
        }
      });
      const r = await ajax('http://fake-host');
      expect(r.data).to.equal('redirect');
    });

    it('should discard post data after redirection', async () => {
      interceptStreamProtocol('http', (request, callback) => {
        if (request.url.indexOf('http://fake-host') === 0) {
          setTimeout(() => {
            callback({
              statusCode: 302,
              headers: {
                Location: 'http://fake-redirect'
              }
            });
          }, 300);
        } else {
          expect(request.url.indexOf('http://fake-redirect')).to.equal(0);
          callback(getStream(3, request.method));
        }
      });
      const r = await ajax('http://fake-host', { type: 'POST', data: postData });
      expect(r.data).to.equal('GET');
    });
  });

  describe('protocol.uninterceptProtocol', () => {
    it('returns false when scheme does not exist', () => {
      expect(uninterceptProtocol('not-exist')).to.equal(false);
    });

    it('returns false when scheme is not intercepted', () => {
      expect(uninterceptProtocol('http')).to.equal(false);
    });
  });

  describe('protocol.registerSchemeAsPrivileged', () => {
    it('does not crash on exit', async () => {
      const appPath = path.join(__dirname, 'fixtures', 'api', 'custom-protocol-shutdown.js');
      const appProcess = ChildProcess.spawn(process.execPath, ['--enable-logging', appPath]);
      let stdout = '';
      let stderr = '';
      appProcess.stdout.on('data', data => { process.stdout.write(data); stdout += data; });
      appProcess.stderr.on('data', data => { process.stderr.write(data); stderr += data; });
      const [code] = await emittedOnce(appProcess, 'exit');
      if (code !== 0) {
        console.log('Exit code : ', code);
        console.log('stdout : ', stdout);
        console.log('stderr : ', stderr);
      }
      expect(code).to.equal(0);
      expect(stdout).to.not.contain('VALIDATION_ERROR_DESERIALIZATION_FAILED');
      expect(stderr).to.not.contain('VALIDATION_ERROR_DESERIALIZATION_FAILED');
    });
  });

  describe('protocol.registerSchemesAsPrivileged allowServiceWorkers', () => {
    protocol.registerStringProtocol(serviceWorkerScheme, (request, cb) => {
      if (request.url.endsWith('.js')) {
        cb({
          mimeType: 'text/javascript',
          charset: 'utf-8',
          data: 'console.log("Loaded")'
        });
      } else {
        cb({
          mimeType: 'text/html',
          charset: 'utf-8',
          data: '<!DOCTYPE html>'
        });
      }
    });
    after(() => protocol.unregisterProtocol(serviceWorkerScheme));

    it('should fail when registering invalid service worker', async () => {
      await contents.loadURL(`${serviceWorkerScheme}://${v4()}.com`);
      await expect(contents.executeJavaScript(`navigator.serviceWorker.register('${v4()}.notjs', {scope: './'})`)).to.be.rejected();
    });

    it('should be able to register service worker for custom scheme', async () => {
      await contents.loadURL(`${serviceWorkerScheme}://${v4()}.com`);
      await contents.executeJavaScript(`navigator.serviceWorker.register('${v4()}.js', {scope: './'})`);
    });
  });

  describe('protocol.registerSchemesAsPrivileged standard', () => {
    const origin = `${standardScheme}://fake-host`;
    const imageURL = `${origin}/test.png`;
    const filePath = path.join(fixturesPath, 'pages', 'b.html');
    const fileContent = '<img src="/test.png" />';
    let w: BrowserWindow = null as unknown as BrowserWindow;

    beforeEach(() => {
      w = new BrowserWindow({
        show: false,
        webPreferences: {
          nodeIntegration: true,
          contextIsolation: false
        }
      });
    });

    afterEach(async () => {
      await closeWindow(w);
      unregisterProtocol(standardScheme);
      w = null as unknown as BrowserWindow;
    });

    it('resolves relative resources', async () => {
      registerFileProtocol(standardScheme, (request, callback) => {
        if (request.url === imageURL) {
          callback('');
        } else {
          callback(filePath);
        }
      });
      await w.loadURL(origin);
    });

    it('resolves absolute resources', async () => {
      registerStringProtocol(standardScheme, (request, callback) => {
        if (request.url === imageURL) {
          callback('');
        } else {
          callback({
            data: fileContent,
            mimeType: 'text/html'
          });
        }
      });
      await w.loadURL(origin);
    });

    it('can have fetch working in it', async () => {
      const requestReceived = defer();
      const server = http.createServer((req, res) => {
        res.end();
        server.close();
        requestReceived.resolve();
      });
      await new Promise<void>(resolve => server.listen(0, '127.0.0.1', resolve));
      const port = (server.address() as AddressInfo).port;
      const content = `<script>fetch("http://127.0.0.1:${port}")</script>`;
      registerStringProtocol(standardScheme, (request, callback) => callback({ data: content, mimeType: 'text/html' }));
      await w.loadURL(origin);
      await requestReceived;
    });

    it.skip('can access files through the FileSystem API', (done) => {
      const filePath = path.join(fixturesPath, 'pages', 'filesystem.html');
      protocol.registerFileProtocol(standardScheme, (request, callback) => callback({ path: filePath }));
      w.loadURL(origin);
      ipcMain.once('file-system-error', (event, err) => done(err));
      ipcMain.once('file-system-write-end', () => done());
    });

    it('registers secure, when {secure: true}', (done) => {
      const filePath = path.join(fixturesPath, 'pages', 'cache-storage.html');
      ipcMain.once('success', () => done());
      ipcMain.once('failure', (event, err) => done(err));
      protocol.registerFileProtocol(standardScheme, (request, callback) => callback({ path: filePath }));
      w.loadURL(origin);
    });
  });

  describe('protocol.registerSchemesAsPrivileged cors-fetch', function () {
    let w: BrowserWindow = null as unknown as BrowserWindow;
    beforeEach(async () => {
      w = new BrowserWindow({ show: false });
    });

    afterEach(async () => {
      await closeWindow(w);
      w = null as unknown as BrowserWindow;
      for (const scheme of [standardScheme, 'cors', 'no-cors', 'no-fetch']) {
        protocol.unregisterProtocol(scheme);
      }
    });

    it('supports fetch api by default', async () => {
      const url = `file://${fixturesPath}/assets/logo.png`;
      await w.loadURL(`file://${fixturesPath}/pages/blank.html`);
      const ok = await w.webContents.executeJavaScript(`fetch(${JSON.stringify(url)}).then(r => r.ok)`);
      expect(ok).to.be.true('response ok');
    });

    it('allows CORS requests by default', async () => {
      await allowsCORSRequests('cors', 200, new RegExp(''), () => {
        const { ipcRenderer } = require('electron');
        fetch('cors://myhost').then(function (response) {
          ipcRenderer.send('response', response.status);
        }).catch(function () {
          ipcRenderer.send('response', 'failed');
        });
      });
    });

    // FIXME: Figure out why this test is failing
    it.skip('disallows CORS and fetch requests when only supportFetchAPI is specified', async () => {
      await allowsCORSRequests('no-cors', ['failed xhr', 'failed fetch'], /has been blocked by CORS policy/, () => {
        const { ipcRenderer } = require('electron');
        Promise.all([
          new Promise(resolve => {
            const req = new XMLHttpRequest();
            req.onload = () => resolve('loaded xhr');
            req.onerror = () => resolve('failed xhr');
            req.open('GET', 'no-cors://myhost');
            req.send();
          }),
          fetch('no-cors://myhost')
            .then(() => 'loaded fetch')
            .catch(() => 'failed fetch')
        ]).then(([xhr, fetch]) => {
          ipcRenderer.send('response', [xhr, fetch]);
        });
      });
    });

    it('allows CORS, but disallows fetch requests, when specified', async () => {
      await allowsCORSRequests('no-fetch', ['loaded xhr', 'failed fetch'], /Fetch API cannot load/, () => {
        const { ipcRenderer } = require('electron');
        Promise.all([
          new Promise(resolve => {
            const req = new XMLHttpRequest();
            req.onload = () => resolve('loaded xhr');
            req.onerror = () => resolve('failed xhr');
            req.open('GET', 'no-fetch://myhost');
            req.send();
          }),
          fetch('no-fetch://myhost')
            .then(() => 'loaded fetch')
            .catch(() => 'failed fetch')
        ]).then(([xhr, fetch]) => {
          ipcRenderer.send('response', [xhr, fetch]);
        });
      });
    });

    async function allowsCORSRequests (corsScheme: string, expected: any, expectedConsole: RegExp, content: Function) {
      registerStringProtocol(standardScheme, (request, callback) => {
        callback({ data: `<script>(${content})()</script>`, mimeType: 'text/html' });
      });
      registerStringProtocol(corsScheme, (request, callback) => {
        callback('');
      });

      const newContents: WebContents = (webContents as any).create({ nodeIntegration: true, contextIsolation: false });
      const consoleMessages: string[] = [];
      newContents.on('console-message', (e, level, message) => consoleMessages.push(message));
      try {
        newContents.loadURL(standardScheme + '://fake-host');
        const [, response] = await emittedOnce(ipcMain, 'response');
        expect(response).to.deep.equal(expected);
        expect(consoleMessages.join('\n')).to.match(expectedConsole);
      } finally {
        // This is called in a timeout to avoid a crash that happens when
        // calling destroy() in a microtask.
        setTimeout(() => {
          (newContents as any).destroy();
        });
      }
    }
  });

  describe('protocol.registerSchemesAsPrivileged stream', async function () {
    const pagePath = path.join(fixturesPath, 'pages', 'video.html');
    const videoSourceImagePath = path.join(fixturesPath, 'video-source-image.webp');
    const videoPath = path.join(fixturesPath, 'video.webm');
    let w: BrowserWindow = null as unknown as BrowserWindow;

    before(async () => {
      // generate test video
      const imageBase64 = await fs.promises.readFile(videoSourceImagePath, 'base64');
      const imageDataUrl = `data:image/webp;base64,${imageBase64}`;
      const encoder = new WebmGenerator(15);
      for (let i = 0; i < 30; i++) {
        encoder.add(imageDataUrl);
      }
      await new Promise((resolve, reject) => {
        encoder.compile((output:Uint8Array) => {
          fs.promises.writeFile(videoPath, output).then(resolve, reject);
        });
      });
    });

    after(async () => {
      await fs.promises.unlink(videoPath);
    });

    beforeEach(async function () {
      w = new BrowserWindow({ show: false });
      await w.loadURL('about:blank');
      if (!await w.webContents.executeJavaScript('document.createElement(\'video\').canPlayType(\'video/webm; codecs="vp8.0"\')')) {
        this.skip();
      }
    });

    afterEach(async () => {
      await closeWindow(w);
      w = null as unknown as BrowserWindow;
      await protocol.unregisterProtocol(standardScheme);
      await protocol.unregisterProtocol('stream');
    });

    it('successfully plays videos when content is buffered (stream: false)', async () => {
      await streamsResponses(standardScheme, 'play');
    });

    it('successfully plays videos when streaming content (stream: true)', async () => {
      await streamsResponses('stream', 'play');
    });

    async function streamsResponses (testingScheme: string, expected: any) {
      const protocolHandler = (request: any, callback: Function) => {
        if (request.url.includes('/video.webm')) {
          const stat = fs.statSync(videoPath);
          const fileSize = stat.size;
          const range = request.headers.Range;
          if (range) {
            const parts = range.replace(/bytes=/, '').split('-');
            const start = parseInt(parts[0], 10);
            const end = parts[1] ? parseInt(parts[1], 10) : fileSize - 1;
            const chunksize = (end - start) + 1;
            const headers = {
              'Content-Range': `bytes ${start}-${end}/${fileSize}`,
              'Accept-Ranges': 'bytes',
              'Content-Length': String(chunksize),
              'Content-Type': 'video/webm'
            };
            callback({ statusCode: 206, headers, data: fs.createReadStream(videoPath, { start, end }) });
          } else {
            callback({
              statusCode: 200,
              headers: {
                'Content-Length': String(fileSize),
                'Content-Type': 'video/webm'
              },
              data: fs.createReadStream(videoPath)
            });
          }
        } else {
          callback({ data: fs.createReadStream(pagePath), headers: { 'Content-Type': 'text/html' }, statusCode: 200 });
        }
      };
      await registerStreamProtocol(standardScheme, protocolHandler);
      await registerStreamProtocol('stream', protocolHandler);

      const newContents: WebContents = (webContents as any).create({ nodeIntegration: true, contextIsolation: false });
      try {
        newContents.loadURL(testingScheme + '://fake-host');
        const [, response] = await emittedOnce(ipcMain, 'result');
        expect(response).to.deep.equal(expected);
      } finally {
        // This is called in a timeout to avoid a crash that happens when
        // calling destroy() in a microtask.
        setTimeout(() => {
          (newContents as any).destroy();
        });
      }
    }
  });
});
