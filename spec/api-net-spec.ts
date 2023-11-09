import { expect } from 'chai';
import * as dns from 'node:dns';
import { net, session, ClientRequest, BrowserWindow, ClientRequestConstructorOptions, protocol } from 'electron/main';
import * as http from 'node:http';
import * as url from 'node:url';
import * as path from 'node:path';
import { Socket } from 'node:net';
import { defer, listen } from './lib/spec-helpers';
import { once } from 'node:events';
import { setTimeout } from 'node:timers/promises';

// See https://github.com/nodejs/node/issues/40702.
dns.setDefaultResultOrder('ipv4first');

const kOneKiloByte = 1024;
const kOneMegaByte = kOneKiloByte * kOneKiloByte;

function randomBuffer (size: number, start: number = 0, end: number = 255) {
  const range = 1 + end - start;
  const buffer = Buffer.allocUnsafe(size);
  for (let i = 0; i < size; ++i) {
    buffer[i] = start + Math.floor(Math.random() * range);
  }
  return buffer;
}

function randomString (length: number) {
  const buffer = randomBuffer(length, '0'.charCodeAt(0), 'z'.charCodeAt(0));
  return buffer.toString();
}

async function getResponse (urlRequest: Electron.ClientRequest) {
  return new Promise<Electron.IncomingMessage>((resolve, reject) => {
    urlRequest.on('error', reject);
    urlRequest.on('abort', reject);
    urlRequest.on('response', (response) => resolve(response));
    urlRequest.end();
  });
}

async function collectStreamBody (response: Electron.IncomingMessage | http.IncomingMessage) {
  return (await collectStreamBodyBuffer(response)).toString();
}

function collectStreamBodyBuffer (response: Electron.IncomingMessage | http.IncomingMessage) {
  return new Promise<Buffer>((resolve, reject) => {
    response.on('error', reject);
    (response as NodeJS.EventEmitter).on('aborted', reject);
    const data: Buffer[] = [];
    response.on('data', (chunk) => data.push(chunk));
    response.on('end', (chunk?: Buffer) => {
      if (chunk) data.push(chunk);
      resolve(Buffer.concat(data));
    });
  });
}

async function respondNTimes (fn: http.RequestListener, n: number): Promise<string> {
  const server = http.createServer((request, response) => {
    fn(request, response);
    // don't close if a redirect was returned
    if ((response.statusCode < 300 || response.statusCode >= 399) && n <= 0) {
      n--;
      server.close();
    }
  });
  const sockets: Socket[] = [];
  server.on('connection', s => sockets.push(s));
  defer(() => {
    server.close();
    for (const socket of sockets) {
      socket.destroy();
    }
  });
  return (await listen(server)).url;
}

function respondOnce (fn: http.RequestListener) {
  return respondNTimes(fn, 1);
}

let routeFailure = false;

respondNTimes.toRoutes = (routes: Record<string, http.RequestListener>, n: number) => {
  return respondNTimes((request, response) => {
    if (Object.hasOwn(routes, request.url || '')) {
      (async () => {
        await Promise.resolve(routes[request.url || ''](request, response));
      })().catch((err) => {
        routeFailure = true;
        console.error('Route handler failed, this is probably why your test failed', err);
        response.statusCode = 500;
        response.end();
      });
    } else {
      response.statusCode = 500;
      response.end();
      expect.fail(`Unexpected URL: ${request.url}`);
    }
  }, n);
};
respondOnce.toRoutes = (routes: Record<string, http.RequestListener>) => respondNTimes.toRoutes(routes, 1);

respondNTimes.toURL = (url: string, fn: http.RequestListener, n: number) => {
  return respondNTimes.toRoutes({ [url]: fn }, n);
};
respondOnce.toURL = (url: string, fn: http.RequestListener) => respondNTimes.toURL(url, fn, 1);

respondNTimes.toSingleURL = (fn: http.RequestListener, n: number) => {
  const requestUrl = '/requestUrl';
  return respondNTimes.toURL(requestUrl, fn, n).then(url => `${url}${requestUrl}`);
};
respondOnce.toSingleURL = (fn: http.RequestListener) => respondNTimes.toSingleURL(fn, 1);

describe('net module', () => {
  beforeEach(() => {
    routeFailure = false;
  });
  afterEach(async function () {
    await session.defaultSession.clearCache();
    if (routeFailure && this.test) {
      if (!this.test.isFailed()) {
        throw new Error('Failing this test due an unhandled error in the respondOnce route handler, check the logs above for the actual error');
      }
    }
  });

  describe('HTTP basics', () => {
    it('should be able to issue a basic GET request', async () => {
      const serverUrl = await respondOnce.toSingleURL((request, response) => {
        expect(request.method).to.equal('GET');
        response.end();
      });
      const urlRequest = net.request(serverUrl);
      const response = await getResponse(urlRequest);
      expect(response.statusCode).to.equal(200);
      await collectStreamBody(response);
    });

    it('should be able to issue a basic POST request', async () => {
      const serverUrl = await respondOnce.toSingleURL((request, response) => {
        expect(request.method).to.equal('POST');
        response.end();
      });
      const urlRequest = net.request({
        method: 'POST',
        url: serverUrl
      });
      const response = await getResponse(urlRequest);
      expect(response.statusCode).to.equal(200);
      await collectStreamBody(response);
    });

    it('should fetch correct data in a GET request', async () => {
      const expectedBodyData = 'Hello World!';
      const serverUrl = await respondOnce.toSingleURL((request, response) => {
        expect(request.method).to.equal('GET');
        response.end(expectedBodyData);
      });
      const urlRequest = net.request(serverUrl);
      const response = await getResponse(urlRequest);
      expect(response.statusCode).to.equal(200);
      const body = await collectStreamBody(response);
      expect(body).to.equal(expectedBodyData);
    });

    it('should post the correct data in a POST request', async () => {
      const bodyData = 'Hello World!';
      let postedBodyData: string = '';
      const serverUrl = await respondOnce.toSingleURL(async (request, response) => {
        postedBodyData = await collectStreamBody(request);
        response.end();
      });
      const urlRequest = net.request({
        method: 'POST',
        url: serverUrl
      });
      urlRequest.write(bodyData);
      const response = await getResponse(urlRequest);
      expect(response.statusCode).to.equal(200);
      expect(postedBodyData).to.equal(bodyData);
    });

    it('a 307 redirected POST request preserves the body', async () => {
      const bodyData = 'Hello World!';
      let postedBodyData: string = '';
      let methodAfterRedirect: string | undefined;
      const serverUrl = await respondNTimes.toRoutes({
        '/redirect': (req, res) => {
          res.statusCode = 307;
          res.setHeader('location', serverUrl);
          return res.end();
        },
        '/': async (req, res) => {
          methodAfterRedirect = req.method;
          postedBodyData = await collectStreamBody(req);
          res.end();
        }
      }, 2);
      const urlRequest = net.request({
        method: 'POST',
        url: serverUrl + '/redirect'
      });
      urlRequest.write(bodyData);
      const response = await getResponse(urlRequest);
      expect(response.statusCode).to.equal(200);
      await collectStreamBody(response);
      expect(methodAfterRedirect).to.equal('POST');
      expect(postedBodyData).to.equal(bodyData);
    });

    it('a 302 redirected POST request DOES NOT preserve the body', async () => {
      const bodyData = 'Hello World!';
      let postedBodyData: string = '';
      let methodAfterRedirect: string | undefined;
      const serverUrl = await respondNTimes.toRoutes({
        '/redirect': (req, res) => {
          res.statusCode = 302;
          res.setHeader('location', serverUrl);
          return res.end();
        },
        '/': async (req, res) => {
          methodAfterRedirect = req.method;
          postedBodyData = await collectStreamBody(req);
          res.end();
        }
      }, 2);
      const urlRequest = net.request({
        method: 'POST',
        url: serverUrl + '/redirect'
      });
      urlRequest.write(bodyData);
      const response = await getResponse(urlRequest);
      expect(response.statusCode).to.equal(200);
      await collectStreamBody(response);
      expect(methodAfterRedirect).to.equal('GET');
      expect(postedBodyData).to.equal('');
    });

    it('should support chunked encoding', async () => {
      let receivedRequest: http.IncomingMessage = null as any;
      const serverUrl = await respondOnce.toSingleURL((request, response) => {
        response.statusCode = 200;
        response.statusMessage = 'OK';
        response.chunkedEncoding = true;
        receivedRequest = request;
        request.on('data', (chunk: Buffer) => {
          response.write(chunk);
        });
        request.on('end', (chunk: Buffer) => {
          response.end(chunk);
        });
      });
      const urlRequest = net.request({
        method: 'POST',
        url: serverUrl
      });

      let chunkIndex = 0;
      const chunkCount = 100;
      let sent = Buffer.alloc(0);

      urlRequest.chunkedEncoding = true;
      while (chunkIndex < chunkCount) {
        chunkIndex += 1;
        const chunk = randomBuffer(kOneKiloByte);
        sent = Buffer.concat([sent, chunk]);
        urlRequest.write(chunk);
      }

      const response = await getResponse(urlRequest);
      expect(receivedRequest.method).to.equal('POST');
      expect(receivedRequest.headers['transfer-encoding']).to.equal('chunked');
      expect(receivedRequest.headers['content-length']).to.equal(undefined);
      expect(response.statusCode).to.equal(200);
      const received = await collectStreamBodyBuffer(response);
      expect(sent.equals(received)).to.be.true();
      expect(chunkIndex).to.be.equal(chunkCount);
    });

    for (const extraOptions of [{}, { credentials: 'include' }, { useSessionCookies: false, credentials: 'include' }] as ClientRequestConstructorOptions[]) {
      describe(`authentication when ${JSON.stringify(extraOptions)}`, () => {
        it('should emit the login event when 401', async () => {
          const [user, pass] = ['user', 'pass'];
          const serverUrl = await respondOnce.toSingleURL((request, response) => {
            if (!request.headers.authorization) {
              return response.writeHead(401, { 'WWW-Authenticate': 'Basic realm="Foo"' }).end();
            }
            response.writeHead(200).end('ok');
          });
          let loginAuthInfo: Electron.AuthInfo;
          const request = net.request({ method: 'GET', url: serverUrl, ...extraOptions });
          request.on('login', (authInfo, cb) => {
            loginAuthInfo = authInfo;
            cb(user, pass);
          });
          const response = await getResponse(request);
          expect(response.statusCode).to.equal(200);
          expect(loginAuthInfo!.realm).to.equal('Foo');
          expect(loginAuthInfo!.scheme).to.equal('basic');
        });

        it('should receive 401 response when cancelling authentication', async () => {
          const serverUrl = await respondOnce.toSingleURL((request, response) => {
            if (!request.headers.authorization) {
              response.writeHead(401, { 'WWW-Authenticate': 'Basic realm="Foo"' });
              response.end('unauthenticated');
            } else {
              response.writeHead(200).end('ok');
            }
          });
          const request = net.request({ method: 'GET', url: serverUrl, ...extraOptions });
          request.on('login', (authInfo, cb) => {
            cb();
          });
          const response = await getResponse(request);
          const body = await collectStreamBody(response);
          expect(response.statusCode).to.equal(401);
          expect(body).to.equal('unauthenticated');
        });

        it('should share credentials with WebContents', async () => {
          const [user, pass] = ['user', 'pass'];
          const serverUrl = await respondNTimes.toSingleURL((request, response) => {
            if (!request.headers.authorization) {
              return response.writeHead(401, { 'WWW-Authenticate': 'Basic realm="Foo"' }).end();
            }
            return response.writeHead(200).end('ok');
          }, 2);
          const bw = new BrowserWindow({ show: false });
          bw.webContents.on('login', (event, details, authInfo, cb) => {
            event.preventDefault();
            cb(user, pass);
          });
          await bw.loadURL(serverUrl);
          bw.close();
          const request = net.request({ method: 'GET', url: serverUrl, ...extraOptions });
          let logInCount = 0;
          request.on('login', () => {
            logInCount++;
          });
          const response = await getResponse(request);
          await collectStreamBody(response);
          expect(logInCount).to.equal(0, 'should not receive a login event, credentials should be cached');
        });

        it('should share proxy credentials with WebContents', async () => {
          const [user, pass] = ['user', 'pass'];
          const proxyUrl = await respondNTimes((request, response) => {
            if (!request.headers['proxy-authorization']) {
              return response.writeHead(407, { 'Proxy-Authenticate': 'Basic realm="Foo"' }).end();
            }
            return response.writeHead(200).end('ok');
          }, 2);
          const customSession = session.fromPartition(`net-proxy-test-${Math.random()}`);
          await customSession.setProxy({ proxyRules: proxyUrl.replace('http://', ''), proxyBypassRules: '<-loopback>' });
          const bw = new BrowserWindow({ show: false, webPreferences: { session: customSession } });
          bw.webContents.on('login', (event, details, authInfo, cb) => {
            event.preventDefault();
            cb(user, pass);
          });
          await bw.loadURL('http://127.0.0.1:9999');
          bw.close();
          const request = net.request({ method: 'GET', url: 'http://127.0.0.1:9999', session: customSession, ...extraOptions });
          let logInCount = 0;
          request.on('login', () => {
            logInCount++;
          });
          const response = await getResponse(request);
          const body = await collectStreamBody(response);
          expect(response.statusCode).to.equal(200);
          expect(body).to.equal('ok');
          expect(logInCount).to.equal(0, 'should not receive a login event, credentials should be cached');
        });

        it('should upload body when 401', async () => {
          const [user, pass] = ['user', 'pass'];
          const serverUrl = await respondOnce.toSingleURL((request, response) => {
            if (!request.headers.authorization) {
              return response.writeHead(401, { 'WWW-Authenticate': 'Basic realm="Foo"' }).end();
            }
            response.writeHead(200);
            request.on('data', (chunk) => response.write(chunk));
            request.on('end', () => response.end());
          });
          const requestData = randomString(kOneKiloByte);
          const request = net.request({ method: 'GET', url: serverUrl, ...extraOptions });
          request.on('login', (authInfo, cb) => {
            cb(user, pass);
          });
          request.write(requestData);
          const response = await getResponse(request);
          const responseData = await collectStreamBody(response);
          expect(responseData).to.equal(requestData);
        });
      });
    }

    describe('authentication when {"credentials":"omit"}', () => {
      it('should not emit the login event when 401', async () => {
        const serverUrl = await respondOnce.toSingleURL((request, response) => {
          if (!request.headers.authorization) {
            return response.writeHead(401, { 'WWW-Authenticate': 'Basic realm="Foo"' }).end();
          }
          response.writeHead(200).end('ok');
        });
        const request = net.request({ method: 'GET', url: serverUrl, credentials: 'omit' });
        request.on('login', () => {
          expect.fail('unexpected login event');
        });
        const response = await getResponse(request);
        expect(response.statusCode).to.equal(401);
        expect(response.headers['www-authenticate']).to.equal('Basic realm="Foo"');
      });

      it('should not share credentials with WebContents', async () => {
        const [user, pass] = ['user', 'pass'];
        const serverUrl = await respondNTimes.toSingleURL((request, response) => {
          if (!request.headers.authorization) {
            return response.writeHead(401, { 'WWW-Authenticate': 'Basic realm="Foo"' }).end();
          }
          return response.writeHead(200).end('ok');
        }, 2);
        const bw = new BrowserWindow({ show: false });
        bw.webContents.on('login', (event, details, authInfo, cb) => {
          event.preventDefault();
          cb(user, pass);
        });
        await bw.loadURL(serverUrl);
        bw.close();
        const request = net.request({ method: 'GET', url: serverUrl, credentials: 'omit' });
        request.on('login', () => {
          expect.fail();
        });
        const response = await getResponse(request);
        expect(response.statusCode).to.equal(401);
        expect(response.headers['www-authenticate']).to.equal('Basic realm="Foo"');
      });

      it('should share proxy credentials with WebContents', async () => {
        const [user, pass] = ['user', 'pass'];
        const proxyUrl = await respondNTimes((request, response) => {
          if (!request.headers['proxy-authorization']) {
            return response.writeHead(407, { 'Proxy-Authenticate': 'Basic realm="Foo"' }).end();
          }
          return response.writeHead(200).end('ok');
        }, 2);
        const customSession = session.fromPartition(`net-proxy-test-${Math.random()}`);
        await customSession.setProxy({ proxyRules: proxyUrl.replace('http://', ''), proxyBypassRules: '<-loopback>' });
        const bw = new BrowserWindow({ show: false, webPreferences: { session: customSession } });
        bw.webContents.on('login', (event, details, authInfo, cb) => {
          event.preventDefault();
          cb(user, pass);
        });
        await bw.loadURL('http://127.0.0.1:9999');
        bw.close();
        const request = net.request({ method: 'GET', url: 'http://127.0.0.1:9999', session: customSession, credentials: 'omit' });
        request.on('login', () => {
          expect.fail();
        });
        const response = await getResponse(request);
        const body = await collectStreamBody(response);
        expect(response.statusCode).to.equal(200);
        expect(body).to.equal('ok');
      });
    });
  });

  describe('ClientRequest API', () => {
    it('request/response objects should emit expected events', async () => {
      const bodyData = randomString(kOneKiloByte);
      const serverUrl = await respondOnce.toSingleURL((request, response) => {
        response.end(bodyData);
      });

      const urlRequest = net.request(serverUrl);
      // request close event
      const closePromise = once(urlRequest, 'close');
      // request finish event
      const finishPromise = once(urlRequest, 'close');
      // request "response" event
      const response = await getResponse(urlRequest);
      response.on('error', (error: Error) => {
        expect(error).to.be.an('Error');
      });
      const statusCode = response.statusCode;
      expect(statusCode).to.equal(200);
      // response data event
      // respond end event
      const body = await collectStreamBody(response);
      expect(body).to.equal(bodyData);
      urlRequest.on('error', (error) => {
        expect(error).to.be.an('Error');
      });
      await Promise.all([closePromise, finishPromise]);
    });

    it('should be able to set a custom HTTP request header before first write', async () => {
      const customHeaderName = 'Some-Custom-Header-Name';
      const customHeaderValue = 'Some-Customer-Header-Value';
      const serverUrl = await respondOnce.toSingleURL((request, response) => {
        expect(request.headers[customHeaderName.toLowerCase()]).to.equal(customHeaderValue);
        response.statusCode = 200;
        response.statusMessage = 'OK';
        response.end();
      });
      const urlRequest = net.request(serverUrl);
      urlRequest.setHeader(customHeaderName, customHeaderValue);
      expect(urlRequest.getHeader(customHeaderName)).to.equal(customHeaderValue);
      expect(urlRequest.getHeader(customHeaderName.toLowerCase())).to.equal(customHeaderValue);
      urlRequest.write('');
      expect(urlRequest.getHeader(customHeaderName)).to.equal(customHeaderValue);
      expect(urlRequest.getHeader(customHeaderName.toLowerCase())).to.equal(customHeaderValue);
      const response = await getResponse(urlRequest);
      expect(response.statusCode).to.equal(200);
      await collectStreamBody(response);
    });

    it('should be able to set a non-string object as a header value', async () => {
      const customHeaderName = 'Some-Integer-Value';
      const customHeaderValue = 900;
      const serverUrl = await respondOnce.toSingleURL((request, response) => {
        expect(request.headers[customHeaderName.toLowerCase()]).to.equal(customHeaderValue.toString());
        response.statusCode = 200;
        response.statusMessage = 'OK';
        response.end();
      });

      const urlRequest = net.request(serverUrl);
      urlRequest.setHeader(customHeaderName, customHeaderValue as any);
      expect(urlRequest.getHeader(customHeaderName)).to.equal(customHeaderValue);
      expect(urlRequest.getHeader(customHeaderName.toLowerCase())).to.equal(customHeaderValue);
      urlRequest.write('');
      expect(urlRequest.getHeader(customHeaderName)).to.equal(customHeaderValue);
      expect(urlRequest.getHeader(customHeaderName.toLowerCase())).to.equal(customHeaderValue);
      const response = await getResponse(urlRequest);
      expect(response.statusCode).to.equal(200);
      await collectStreamBody(response);
    });

    it('should not change the case of header name', async () => {
      const customHeaderName = 'X-Header-Name';
      const customHeaderValue = 'value';
      const serverUrl = await respondOnce.toSingleURL((request, response) => {
        expect(request.headers[customHeaderName.toLowerCase()]).to.equal(customHeaderValue.toString());
        expect(request.rawHeaders.includes(customHeaderName)).to.equal(true);
        response.statusCode = 200;
        response.statusMessage = 'OK';
        response.end();
      });

      const urlRequest = net.request(serverUrl);
      urlRequest.setHeader(customHeaderName, customHeaderValue);
      expect(urlRequest.getHeader(customHeaderName)).to.equal(customHeaderValue);
      urlRequest.write('');
      const response = await getResponse(urlRequest);
      expect(response.statusCode).to.equal(200);
      await collectStreamBody(response);
    });

    it('should not be able to set a custom HTTP request header after first write', async () => {
      const customHeaderName = 'Some-Custom-Header-Name';
      const customHeaderValue = 'Some-Customer-Header-Value';
      const serverUrl = await respondOnce.toSingleURL((request, response) => {
        expect(request.headers[customHeaderName.toLowerCase()]).to.equal(undefined);
        response.statusCode = 200;
        response.statusMessage = 'OK';
        response.end();
      });
      const urlRequest = net.request(serverUrl);
      urlRequest.write('');
      expect(() => {
        urlRequest.setHeader(customHeaderName, customHeaderValue);
      }).to.throw();
      expect(urlRequest.getHeader(customHeaderName)).to.equal(undefined);
      const response = await getResponse(urlRequest);
      expect(response.statusCode).to.equal(200);
      await collectStreamBody(response);
    });

    it('should be able to remove a custom HTTP request header before first write', async () => {
      const customHeaderName = 'Some-Custom-Header-Name';
      const customHeaderValue = 'Some-Customer-Header-Value';
      const serverUrl = await respondOnce.toSingleURL((request, response) => {
        expect(request.headers[customHeaderName.toLowerCase()]).to.equal(undefined);
        response.statusCode = 200;
        response.statusMessage = 'OK';
        response.end();
      });
      const urlRequest = net.request(serverUrl);
      urlRequest.setHeader(customHeaderName, customHeaderValue);
      expect(urlRequest.getHeader(customHeaderName)).to.equal(customHeaderValue);
      urlRequest.removeHeader(customHeaderName);
      expect(urlRequest.getHeader(customHeaderName)).to.equal(undefined);
      urlRequest.write('');
      const response = await getResponse(urlRequest);
      expect(response.statusCode).to.equal(200);
      await collectStreamBody(response);
    });

    it('should not be able to remove a custom HTTP request header after first write', async () => {
      const customHeaderName = 'Some-Custom-Header-Name';
      const customHeaderValue = 'Some-Customer-Header-Value';
      const serverUrl = await respondOnce.toSingleURL((request, response) => {
        expect(request.headers[customHeaderName.toLowerCase()]).to.equal(customHeaderValue);
        response.statusCode = 200;
        response.statusMessage = 'OK';
        response.end();
      });
      const urlRequest = net.request(serverUrl);
      urlRequest.setHeader(customHeaderName, customHeaderValue);
      expect(urlRequest.getHeader(customHeaderName)).to.equal(customHeaderValue);
      urlRequest.write('');
      expect(() => {
        urlRequest.removeHeader(customHeaderName);
      }).to.throw();
      expect(urlRequest.getHeader(customHeaderName)).to.equal(customHeaderValue);
      const response = await getResponse(urlRequest);
      expect(response.statusCode).to.equal(200);
      await collectStreamBody(response);
    });

    it('should keep the order of headers', async () => {
      const customHeaderNameA = 'X-Header-100';
      const customHeaderNameB = 'X-Header-200';
      const serverUrl = await respondOnce.toSingleURL((request, response) => {
        const headerNames = Array.from(Object.keys(request.headers));
        const headerAIndex = headerNames.indexOf(customHeaderNameA.toLowerCase());
        const headerBIndex = headerNames.indexOf(customHeaderNameB.toLowerCase());
        expect(headerBIndex).to.be.below(headerAIndex);
        response.statusCode = 200;
        response.statusMessage = 'OK';
        response.end();
      });

      const urlRequest = net.request(serverUrl);
      urlRequest.setHeader(customHeaderNameB, 'b');
      urlRequest.setHeader(customHeaderNameA, 'a');
      const response = await getResponse(urlRequest);
      expect(response.statusCode).to.equal(200);
      await collectStreamBody(response);
    });

    it('should be able to set cookie header line', async () => {
      const cookieHeaderName = 'Cookie';
      const cookieHeaderValue = 'test=12345';
      const customSession = session.fromPartition(`test-cookie-header-${Math.random()}`);
      const serverUrl = await respondOnce.toSingleURL((request, response) => {
        expect(request.headers[cookieHeaderName.toLowerCase()]).to.equal(cookieHeaderValue);
        response.statusCode = 200;
        response.statusMessage = 'OK';
        response.end();
      });
      await customSession.cookies.set({
        url: `${serverUrl}`,
        name: 'test',
        value: '11111',
        expirationDate: 0
      });
      const urlRequest = net.request({
        method: 'GET',
        url: serverUrl,
        session: customSession
      });
      urlRequest.setHeader(cookieHeaderName, cookieHeaderValue);
      expect(urlRequest.getHeader(cookieHeaderName)).to.equal(cookieHeaderValue);
      const response = await getResponse(urlRequest);
      expect(response.statusCode).to.equal(200);
      await collectStreamBody(response);
    });

    it('should be able to receive cookies', async () => {
      const cookie = ['cookie1', 'cookie2'];
      const serverUrl = await respondOnce.toSingleURL((request, response) => {
        response.statusCode = 200;
        response.statusMessage = 'OK';
        response.setHeader('set-cookie', cookie);
        response.end();
      });
      const urlRequest = net.request(serverUrl);
      const response = await getResponse(urlRequest);
      expect(response.headers['set-cookie']).to.have.same.members(cookie);
    });

    it('should be able to receive content-type', async () => {
      const contentType = 'mime/test; charset=test';
      const serverUrl = await respondOnce.toSingleURL((request, response) => {
        response.statusCode = 200;
        response.statusMessage = 'OK';
        response.setHeader('content-type', contentType);
        response.end();
      });
      const urlRequest = net.request(serverUrl);
      const response = await getResponse(urlRequest);
      expect(response.headers['content-type']).to.equal(contentType);
    });

    it('should not use the sessions cookie store by default', async () => {
      const serverUrl = await respondOnce.toSingleURL((request, response) => {
        response.statusCode = 200;
        response.statusMessage = 'OK';
        response.setHeader('x-cookie', `${request.headers.cookie!}`);
        response.end();
      });
      const sess = session.fromPartition(`cookie-tests-${Math.random()}`);
      const cookieVal = `${Date.now()}`;
      await sess.cookies.set({
        url: serverUrl,
        name: 'wild_cookie',
        value: cookieVal
      });
      const urlRequest = net.request({
        url: serverUrl,
        session: sess
      });
      const response = await getResponse(urlRequest);
      expect(response.headers['x-cookie']).to.equal('undefined');
    });

    for (const extraOptions of [{ useSessionCookies: true }, { credentials: 'include' }] as ClientRequestConstructorOptions[]) {
      describe(`when ${JSON.stringify(extraOptions)}`, () => {
        it('should be able to use the sessions cookie store', async () => {
          const serverUrl = await respondOnce.toSingleURL((request, response) => {
            response.statusCode = 200;
            response.statusMessage = 'OK';
            response.setHeader('x-cookie', request.headers.cookie!);
            response.end();
          });
          const sess = session.fromPartition(`cookie-tests-${Math.random()}`);
          const cookieVal = `${Date.now()}`;
          await sess.cookies.set({
            url: serverUrl,
            name: 'wild_cookie',
            value: cookieVal
          });
          const urlRequest = net.request({
            url: serverUrl,
            session: sess,
            ...extraOptions
          });
          const response = await getResponse(urlRequest);
          expect(response.headers['x-cookie']).to.equal(`wild_cookie=${cookieVal}`);
        });

        it('should be able to use the sessions cookie store with set-cookie', async () => {
          const serverUrl = await respondOnce.toSingleURL((request, response) => {
            response.statusCode = 200;
            response.statusMessage = 'OK';
            response.setHeader('set-cookie', 'foo=bar');
            response.end();
          });
          const sess = session.fromPartition(`cookie-tests-${Math.random()}`);
          let cookies = await sess.cookies.get({});
          expect(cookies).to.have.lengthOf(0);
          const urlRequest = net.request({
            url: serverUrl,
            session: sess,
            ...extraOptions
          });
          await collectStreamBody(await getResponse(urlRequest));
          cookies = await sess.cookies.get({});
          expect(cookies).to.have.lengthOf(1);
          expect(cookies[0]).to.deep.equal({
            name: 'foo',
            value: 'bar',
            domain: '127.0.0.1',
            hostOnly: true,
            path: '/',
            secure: false,
            httpOnly: false,
            session: true,
            sameSite: 'unspecified'
          });
        });

        for (const mode of ['Lax', 'Strict']) {
          it(`should be able to use the sessions cookie store with same-site ${mode} cookies`, async () => {
            const serverUrl = await respondNTimes.toSingleURL((request, response) => {
              response.statusCode = 200;
              response.statusMessage = 'OK';
              response.setHeader('set-cookie', `same=site; SameSite=${mode}`);
              response.setHeader('x-cookie', `${request.headers.cookie}`);
              response.end();
            }, 2);
            const sess = session.fromPartition(`cookie-tests-${Math.random()}`);
            let cookies = await sess.cookies.get({});
            expect(cookies).to.have.lengthOf(0);
            const urlRequest = net.request({
              url: serverUrl,
              session: sess,
              ...extraOptions
            });
            const response = await getResponse(urlRequest);
            expect(response.headers['x-cookie']).to.equal('undefined');
            await collectStreamBody(response);
            cookies = await sess.cookies.get({});
            expect(cookies).to.have.lengthOf(1);
            expect(cookies[0]).to.deep.equal({
              name: 'same',
              value: 'site',
              domain: '127.0.0.1',
              hostOnly: true,
              path: '/',
              secure: false,
              httpOnly: false,
              session: true,
              sameSite: mode.toLowerCase()
            });
            const urlRequest2 = net.request({
              url: serverUrl,
              session: sess,
              ...extraOptions
            });
            const response2 = await getResponse(urlRequest2);
            expect(response2.headers['x-cookie']).to.equal('same=site');
          });
        }

        it('should be able to use the sessions cookie store safely across redirects', async () => {
          const serverUrl = await respondOnce.toSingleURL(async (request, response) => {
            response.statusCode = 302;
            response.statusMessage = 'Moved';
            const newUrl = await respondOnce.toSingleURL((req, res) => {
              res.statusCode = 200;
              res.statusMessage = 'OK';
              res.setHeader('x-cookie', req.headers.cookie!);
              res.end();
            });
            response.setHeader('x-cookie', request.headers.cookie!);
            response.setHeader('location', newUrl.replace('127.0.0.1', 'localhost'));
            response.end();
          });
          const sess = session.fromPartition(`cookie-tests-${Math.random()}`);
          const cookie127Val = `${Date.now()}-127`;
          const cookieLocalVal = `${Date.now()}-local`;
          const localhostUrl = serverUrl.replace('127.0.0.1', 'localhost');
          expect(localhostUrl).to.not.equal(serverUrl);
          // cookies with lax or strict same-site settings will not
          // persist after redirects. no_restriction must be used
          await Promise.all([
            sess.cookies.set({
              url: serverUrl,
              name: 'wild_cookie',
              sameSite: 'no_restriction',
              value: cookie127Val
            }), sess.cookies.set({
              url: localhostUrl,
              name: 'wild_cookie',
              sameSite: 'no_restriction',
              value: cookieLocalVal
            })
          ]);
          const urlRequest = net.request({
            url: serverUrl,
            session: sess,
            ...extraOptions
          });
          urlRequest.on('redirect', (status, method, url, headers) => {
            // The initial redirect response should have received the 127 value here
            expect(headers['x-cookie'][0]).to.equal(`wild_cookie=${cookie127Val}`);
            urlRequest.followRedirect();
          });
          const response = await getResponse(urlRequest);
          // We expect the server to have received the localhost value here
          // The original request was to a 127.0.0.1 URL
          // That request would have the cookie127Val cookie attached
          // The request is then redirect to a localhost URL (different site)
          // Because we are using the session cookie store it should do the safe / secure thing
          // and attach the cookies for the new target domain
          expect(response.headers['x-cookie']).to.equal(`wild_cookie=${cookieLocalVal}`);
        });
      });
    }

    it('should be able correctly filter out cookies that are secure', async () => {
      const sess = session.fromPartition(`cookie-tests-${Math.random()}`);

      await Promise.all([
        sess.cookies.set({
          url: 'https://electronjs.org',
          domain: 'electronjs.org',
          name: 'cookie1',
          value: '1',
          secure: true
        }),
        sess.cookies.set({
          url: 'https://electronjs.org',
          domain: 'electronjs.org',
          name: 'cookie2',
          value: '2',
          secure: false
        })
      ]);

      const secureCookies = await sess.cookies.get({
        secure: true
      });
      expect(secureCookies).to.have.lengthOf(1);
      expect(secureCookies[0].name).to.equal('cookie1');

      const cookies = await sess.cookies.get({
        secure: false
      });
      expect(cookies).to.have.lengthOf(1);
      expect(cookies[0].name).to.equal('cookie2');
    });

    it('throws when an invalid domain is passed', async () => {
      const sess = session.fromPartition(`cookie-tests-${Math.random()}`);

      await expect(sess.cookies.set({
        url: 'https://electronjs.org',
        domain: 'wssss.iamabaddomain.fun',
        name: 'cookie1'
      })).to.eventually.be.rejectedWith(/Failed to set cookie with an invalid domain attribute/);
    });

    it('should be able correctly filter out cookies that are session', async () => {
      const sess = session.fromPartition(`cookie-tests-${Math.random()}`);

      await Promise.all([
        sess.cookies.set({
          url: 'https://electronjs.org',
          domain: 'electronjs.org',
          name: 'cookie1',
          value: '1'
        }),
        sess.cookies.set({
          url: 'https://electronjs.org',
          domain: 'electronjs.org',
          name: 'cookie2',
          value: '2',
          expirationDate: Math.round(Date.now() / 1000) + 10000
        })
      ]);

      const sessionCookies = await sess.cookies.get({
        session: true
      });
      expect(sessionCookies).to.have.lengthOf(1);
      expect(sessionCookies[0].name).to.equal('cookie1');

      const cookies = await sess.cookies.get({
        session: false
      });
      expect(cookies).to.have.lengthOf(1);
      expect(cookies[0].name).to.equal('cookie2');
    });

    it('should be able correctly filter out cookies that are httpOnly', async () => {
      const sess = session.fromPartition(`cookie-tests-${Math.random()}`);

      await Promise.all([
        sess.cookies.set({
          url: 'https://electronjs.org',
          domain: 'electronjs.org',
          name: 'cookie1',
          value: '1',
          httpOnly: true
        }),
        sess.cookies.set({
          url: 'https://electronjs.org',
          domain: 'electronjs.org',
          name: 'cookie2',
          value: '2',
          httpOnly: false
        })
      ]);

      const httpOnlyCookies = await sess.cookies.get({
        httpOnly: true
      });
      expect(httpOnlyCookies).to.have.lengthOf(1);
      expect(httpOnlyCookies[0].name).to.equal('cookie1');

      const cookies = await sess.cookies.get({
        httpOnly: false
      });
      expect(cookies).to.have.lengthOf(1);
      expect(cookies[0].name).to.equal('cookie2');
    });

    describe('when {"credentials":"omit"}', () => {
      it('should not send cookies');
      it('should not store cookies');
    });

    it('should set sec-fetch-site to same-origin for request from same origin', async () => {
      const serverUrl = await respondOnce.toSingleURL((request, response) => {
        expect(request.headers['sec-fetch-site']).to.equal('same-origin');
        response.statusCode = 200;
        response.statusMessage = 'OK';
        response.end();
      });
      const urlRequest = net.request({
        url: serverUrl,
        origin: serverUrl
      });
      await collectStreamBody(await getResponse(urlRequest));
    });

    it('should set sec-fetch-site to same-origin for request with the same origin header', async () => {
      const serverUrl = await respondOnce.toSingleURL((request, response) => {
        expect(request.headers['sec-fetch-site']).to.equal('same-origin');
        response.statusCode = 200;
        response.statusMessage = 'OK';
        response.end();
      });
      const urlRequest = net.request({
        url: serverUrl
      });
      urlRequest.setHeader('Origin', serverUrl);
      await collectStreamBody(await getResponse(urlRequest));
    });

    it('should set sec-fetch-site to cross-site for request from other origin', async () => {
      const serverUrl = await respondOnce.toSingleURL((request, response) => {
        expect(request.headers['sec-fetch-site']).to.equal('cross-site');
        response.statusCode = 200;
        response.statusMessage = 'OK';
        response.end();
      });
      const urlRequest = net.request({
        url: serverUrl,
        origin: 'https://not-exists.com'
      });
      await collectStreamBody(await getResponse(urlRequest));
    });

    it('should not send sec-fetch-user header by default', async () => {
      const serverUrl = await respondOnce.toSingleURL((request, response) => {
        expect(request.headers).not.to.have.property('sec-fetch-user');
        response.statusCode = 200;
        response.statusMessage = 'OK';
        response.end();
      });
      const urlRequest = net.request({
        url: serverUrl
      });
      await collectStreamBody(await getResponse(urlRequest));
    });

    it('should set sec-fetch-user to ?1 if requested', async () => {
      const serverUrl = await respondOnce.toSingleURL((request, response) => {
        expect(request.headers['sec-fetch-user']).to.equal('?1');
        response.statusCode = 200;
        response.statusMessage = 'OK';
        response.end();
      });
      const urlRequest = net.request({
        url: serverUrl
      });
      urlRequest.setHeader('sec-fetch-user', '?1');
      await collectStreamBody(await getResponse(urlRequest));
    });

    it('should set sec-fetch-mode to no-cors by default', async () => {
      const serverUrl = await respondOnce.toSingleURL((request, response) => {
        expect(request.headers['sec-fetch-mode']).to.equal('no-cors');
        response.statusCode = 200;
        response.statusMessage = 'OK';
        response.end();
      });
      const urlRequest = net.request({
        url: serverUrl
      });
      await collectStreamBody(await getResponse(urlRequest));
    });

    for (const mode of ['navigate', 'cors', 'no-cors', 'same-origin']) {
      it(`should set sec-fetch-mode to ${mode} if requested`, async () => {
        const serverUrl = await respondOnce.toSingleURL((request, response) => {
          expect(request.headers['sec-fetch-mode']).to.equal(mode);
          response.statusCode = 200;
          response.statusMessage = 'OK';
          response.end();
        });
        const urlRequest = net.request({
          url: serverUrl,
          origin: serverUrl
        });
        urlRequest.setHeader('sec-fetch-mode', mode);
        await collectStreamBody(await getResponse(urlRequest));
      });
    }

    it('should set sec-fetch-dest to empty by default', async () => {
      const serverUrl = await respondOnce.toSingleURL((request, response) => {
        expect(request.headers['sec-fetch-dest']).to.equal('empty');
        response.statusCode = 200;
        response.statusMessage = 'OK';
        response.end();
      });
      const urlRequest = net.request({
        url: serverUrl
      });
      await collectStreamBody(await getResponse(urlRequest));
    });

    for (const dest of [
      'empty', 'audio', 'audioworklet', 'document', 'embed', 'font',
      'frame', 'iframe', 'image', 'manifest', 'object', 'paintworklet',
      'report', 'script', 'serviceworker', 'style', 'track', 'video',
      'worker', 'xslt'
    ]) {
      it(`should set sec-fetch-dest to ${dest} if requested`, async () => {
        const serverUrl = await respondOnce.toSingleURL((request, response) => {
          expect(request.headers['sec-fetch-dest']).to.equal(dest);
          response.statusCode = 200;
          response.statusMessage = 'OK';
          response.end();
        });
        const urlRequest = net.request({
          url: serverUrl,
          origin: serverUrl
        });
        urlRequest.setHeader('sec-fetch-dest', dest);
        await collectStreamBody(await getResponse(urlRequest));
      });
    }

    it('should be able to abort an HTTP request before first write', async () => {
      const serverUrl = await respondOnce.toSingleURL((request, response) => {
        response.end();
        expect.fail('Unexpected request event');
      });

      const urlRequest = net.request(serverUrl);
      urlRequest.on('response', () => {
        expect.fail('unexpected response event');
      });
      const aborted = once(urlRequest, 'abort');
      urlRequest.abort();
      urlRequest.write('');
      urlRequest.end();
      await aborted;
    });

    it('it should be able to abort an HTTP request before request end', async () => {
      let requestReceivedByServer = false;
      let urlRequest: ClientRequest | null = null;
      const serverUrl = await respondOnce.toSingleURL(() => {
        requestReceivedByServer = true;
        urlRequest!.abort();
      });
      let requestAbortEventEmitted = false;

      urlRequest = net.request(serverUrl);
      urlRequest.on('response', () => {
        expect.fail('Unexpected response event');
      });
      urlRequest.on('finish', () => {
        expect.fail('Unexpected finish event');
      });
      urlRequest.on('error', () => {
        expect.fail('Unexpected error event');
      });
      urlRequest.on('abort', () => {
        requestAbortEventEmitted = true;
      });

      const p = once(urlRequest, 'close');
      urlRequest.chunkedEncoding = true;
      urlRequest.write(randomString(kOneKiloByte));
      await p;
      expect(requestReceivedByServer).to.equal(true);
      expect(requestAbortEventEmitted).to.equal(true);
    });

    it('it should be able to abort an HTTP request after request end and before response', async () => {
      let requestReceivedByServer = false;
      let urlRequest: ClientRequest | null = null;
      const serverUrl = await respondOnce.toSingleURL((request, response) => {
        requestReceivedByServer = true;
        urlRequest!.abort();
        process.nextTick(() => {
          response.statusCode = 200;
          response.statusMessage = 'OK';
          response.end();
        });
      });
      let requestFinishEventEmitted = false;

      urlRequest = net.request(serverUrl);
      urlRequest.on('response', () => {
        expect.fail('Unexpected response event');
      });
      urlRequest.on('finish', () => {
        requestFinishEventEmitted = true;
      });
      urlRequest.on('error', () => {
        expect.fail('Unexpected error event');
      });
      urlRequest.end(randomString(kOneKiloByte));
      await once(urlRequest, 'abort');
      expect(requestFinishEventEmitted).to.equal(true);
      expect(requestReceivedByServer).to.equal(true);
    });

    it('it should be able to abort an HTTP request after response start', async () => {
      let requestReceivedByServer = false;
      const serverUrl = await respondOnce.toSingleURL((request, response) => {
        requestReceivedByServer = true;
        response.statusCode = 200;
        response.statusMessage = 'OK';
        response.write(randomString(kOneKiloByte));
      });
      let requestFinishEventEmitted = false;
      let requestResponseEventEmitted = false;
      let responseCloseEventEmitted = false;

      const urlRequest = net.request(serverUrl);
      urlRequest.on('response', (response) => {
        requestResponseEventEmitted = true;
        const statusCode = response.statusCode;
        expect(statusCode).to.equal(200);
        response.on('data', () => {});
        response.on('end', () => {
          expect.fail('Unexpected end event');
        });
        response.on('error', () => {
          expect.fail('Unexpected error event');
        });
        response.on('close' as any, () => {
          responseCloseEventEmitted = true;
        });
        urlRequest.abort();
      });
      urlRequest.on('finish', () => {
        requestFinishEventEmitted = true;
      });
      urlRequest.on('error', () => {
        expect.fail('Unexpected error event');
      });
      urlRequest.end(randomString(kOneKiloByte));
      await once(urlRequest, 'abort');
      expect(requestFinishEventEmitted).to.be.true('request should emit "finish" event');
      expect(requestReceivedByServer).to.be.true('request should be received by the server');
      expect(requestResponseEventEmitted).to.be.true('"response" event should be emitted');
      expect(responseCloseEventEmitted).to.be.true('response should emit "close" event');
    });

    it('abort event should be emitted at most once', async () => {
      let requestReceivedByServer = false;
      let urlRequest: ClientRequest | null = null;
      const serverUrl = await respondOnce.toSingleURL(() => {
        requestReceivedByServer = true;
        urlRequest!.abort();
        urlRequest!.abort();
      });
      let requestFinishEventEmitted = false;
      let abortsEmitted = 0;

      urlRequest = net.request(serverUrl);
      urlRequest.on('response', () => {
        expect.fail('Unexpected response event');
      });
      urlRequest.on('finish', () => {
        requestFinishEventEmitted = true;
      });
      urlRequest.on('error', () => {
        expect.fail('Unexpected error event');
      });
      urlRequest.on('abort', () => {
        abortsEmitted++;
      });
      urlRequest.end(randomString(kOneKiloByte));
      await once(urlRequest, 'abort');
      expect(requestFinishEventEmitted).to.be.true('request should emit "finish" event');
      expect(requestReceivedByServer).to.be.true('request should be received by server');
      expect(abortsEmitted).to.equal(1, 'request should emit exactly 1 "abort" event');
    });

    it('should allow to read response body from non-2xx response', async () => {
      const bodyData = randomString(kOneKiloByte);
      const serverUrl = await respondOnce.toSingleURL((request, response) => {
        response.statusCode = 404;
        response.end(bodyData);
      });

      const urlRequest = net.request(serverUrl);
      const bodyCheckPromise = getResponse(urlRequest).then(r => {
        expect(r.statusCode).to.equal(404);
        return r;
      }).then(collectStreamBody).then(receivedBodyData => {
        expect(receivedBodyData.toString()).to.equal(bodyData);
      });
      const eventHandlers = Promise.all([
        bodyCheckPromise,
        once(urlRequest, 'close')
      ]);

      urlRequest.end();

      await eventHandlers;
    });

    describe('webRequest', () => {
      afterEach(() => {
        session.defaultSession.webRequest.onBeforeRequest(null);
      });

      it('Should throw when invalid filters are passed', () => {
        expect(() => {
          session.defaultSession.webRequest.onBeforeRequest(
            { urls: ['*://www.googleapis.com'] },
            (details, callback) => { callback({ cancel: false }); }
          );
        }).to.throw('Invalid url pattern *://www.googleapis.com: Empty path.');

        expect(() => {
          session.defaultSession.webRequest.onBeforeRequest(
            { urls: ['*://www.googleapis.com/', '*://blahblah.dev'] },
            (details, callback) => { callback({ cancel: false }); }
          );
        }).to.throw('Invalid url pattern *://blahblah.dev: Empty path.');
      });

      it('Should not throw when valid filters are passed', () => {
        expect(() => {
          session.defaultSession.webRequest.onBeforeRequest(
            { urls: ['*://www.googleapis.com/'] },
            (details, callback) => { callback({ cancel: false }); }
          );
        }).to.not.throw();
      });

      it('Requests should be intercepted by webRequest module', async () => {
        const requestUrl = '/requestUrl';
        const redirectUrl = '/redirectUrl';
        let requestIsRedirected = false;
        const serverUrl = await respondOnce.toURL(redirectUrl, (request, response) => {
          requestIsRedirected = true;
          response.end();
        });
        let requestIsIntercepted = false;
        session.defaultSession.webRequest.onBeforeRequest(
          (details, callback) => {
            if (details.url === `${serverUrl}${requestUrl}`) {
              requestIsIntercepted = true;
              callback({
                redirectURL: `${serverUrl}${redirectUrl}`
              });
            } else {
              callback({
                cancel: false
              });
            }
          });

        const urlRequest = net.request(`${serverUrl}${requestUrl}`);
        const response = await getResponse(urlRequest);

        expect(response.statusCode).to.equal(200);
        await collectStreamBody(response);
        expect(requestIsRedirected).to.be.true('The server should receive a request to the forward URL');
        expect(requestIsIntercepted).to.be.true('The request should be intercepted by the webRequest module');
      });

      it('should to able to create and intercept a request using a custom session object', async () => {
        const requestUrl = '/requestUrl';
        const redirectUrl = '/redirectUrl';
        const customPartitionName = `custom-partition-${Math.random()}`;
        let requestIsRedirected = false;
        const serverUrl = await respondOnce.toURL(redirectUrl, (request, response) => {
          requestIsRedirected = true;
          response.end();
        });
        session.defaultSession.webRequest.onBeforeRequest(() => {
          expect.fail('Request should not be intercepted by the default session');
        });

        const customSession = session.fromPartition(customPartitionName, { cache: false });
        let requestIsIntercepted = false;
        customSession.webRequest.onBeforeRequest((details, callback) => {
          if (details.url === `${serverUrl}${requestUrl}`) {
            requestIsIntercepted = true;
            callback({
              redirectURL: `${serverUrl}${redirectUrl}`
            });
          } else {
            callback({
              cancel: false
            });
          }
        });

        const urlRequest = net.request({
          url: `${serverUrl}${requestUrl}`,
          session: customSession
        });
        const response = await getResponse(urlRequest);
        expect(response.statusCode).to.equal(200);
        await collectStreamBody(response);
        expect(requestIsRedirected).to.be.true('The server should receive a request to the forward URL');
        expect(requestIsIntercepted).to.be.true('The request should be intercepted by the webRequest module');
      });

      it('should to able to create and intercept a request using a custom partition name', async () => {
        const requestUrl = '/requestUrl';
        const redirectUrl = '/redirectUrl';
        const customPartitionName = `custom-partition-${Math.random()}`;
        let requestIsRedirected = false;
        const serverUrl = await respondOnce.toURL(redirectUrl, (request, response) => {
          requestIsRedirected = true;
          response.end();
        });
        session.defaultSession.webRequest.onBeforeRequest(() => {
          expect.fail('Request should not be intercepted by the default session');
        });

        const customSession = session.fromPartition(customPartitionName, { cache: false });
        let requestIsIntercepted = false;
        customSession.webRequest.onBeforeRequest((details, callback) => {
          if (details.url === `${serverUrl}${requestUrl}`) {
            requestIsIntercepted = true;
            callback({
              redirectURL: `${serverUrl}${redirectUrl}`
            });
          } else {
            callback({
              cancel: false
            });
          }
        });

        const urlRequest = net.request({
          url: `${serverUrl}${requestUrl}`,
          partition: customPartitionName
        });
        const response = await getResponse(urlRequest);
        expect(response.statusCode).to.equal(200);
        await collectStreamBody(response);
        expect(requestIsRedirected).to.be.true('The server should receive a request to the forward URL');
        expect(requestIsIntercepted).to.be.true('The request should be intercepted by the webRequest module');
      });

      it('triggers webRequest handlers when bypassCustomProtocolHandlers', async () => {
        let webRequestDetails: Electron.OnBeforeRequestListenerDetails | null = null;
        const serverUrl = await respondOnce.toSingleURL((req, res) => res.end('hi'));
        session.defaultSession.webRequest.onBeforeRequest((details, cb) => {
          webRequestDetails = details;
          cb({});
        });
        const body = await net.fetch(serverUrl, { bypassCustomProtocolHandlers: true }).then(r => r.text());
        expect(body).to.equal('hi');
        expect(webRequestDetails).to.have.property('url', serverUrl);
      });
    });

    it('should throw when calling getHeader without a name', () => {
      expect(() => {
        (net.request({ url: 'https://test' }).getHeader as any)();
      }).to.throw(/`name` is required for getHeader\(name\)/);

      expect(() => {
        net.request({ url: 'https://test' }).getHeader(null as any);
      }).to.throw(/`name` is required for getHeader\(name\)/);
    });

    it('should throw when calling removeHeader without a name', () => {
      expect(() => {
        (net.request({ url: 'https://test' }).removeHeader as any)();
      }).to.throw(/`name` is required for removeHeader\(name\)/);

      expect(() => {
        net.request({ url: 'https://test' }).removeHeader(null as any);
      }).to.throw(/`name` is required for removeHeader\(name\)/);
    });

    it('should follow redirect when no redirect handler is provided', async () => {
      const requestUrl = '/302';
      const serverUrl = await respondOnce.toRoutes({
        '/302': (request, response) => {
          response.statusCode = 302;
          response.setHeader('Location', '/200');
          response.end();
        },
        '/200': (request, response) => {
          response.statusCode = 200;
          response.end();
        }
      });
      const urlRequest = net.request({
        url: `${serverUrl}${requestUrl}`
      });
      const response = await getResponse(urlRequest);
      expect(response.statusCode).to.equal(200);
    });

    it('should follow redirect chain when no redirect handler is provided', async () => {
      const serverUrl = await respondOnce.toRoutes({
        '/redirectChain': (request, response) => {
          response.statusCode = 302;
          response.setHeader('Location', '/302');
          response.end();
        },
        '/302': (request, response) => {
          response.statusCode = 302;
          response.setHeader('Location', '/200');
          response.end();
        },
        '/200': (request, response) => {
          response.statusCode = 200;
          response.end();
        }
      });
      const urlRequest = net.request({
        url: `${serverUrl}/redirectChain`
      });
      const response = await getResponse(urlRequest);
      expect(response.statusCode).to.equal(200);
    });

    it('should not follow redirect when request is canceled in redirect handler', async () => {
      const serverUrl = await respondOnce.toSingleURL((request, response) => {
        response.statusCode = 302;
        response.setHeader('Location', '/200');
        response.end();
      });
      const urlRequest = net.request({
        url: serverUrl
      });
      urlRequest.end();
      urlRequest.on('redirect', () => { urlRequest.abort(); });
      urlRequest.on('error', () => {});
      urlRequest.on('response', () => {
        expect.fail('Unexpected response');
      });
      await once(urlRequest, 'abort');
    });

    it('should not follow redirect when mode is error', async () => {
      const serverUrl = await respondOnce.toSingleURL((request, response) => {
        response.statusCode = 302;
        response.setHeader('Location', '/200');
        response.end();
      });
      const urlRequest = net.request({
        url: serverUrl,
        redirect: 'error'
      });
      urlRequest.end();
      await once(urlRequest, 'error');
    });

    it('should follow redirect when handler calls callback', async () => {
      const serverUrl = await respondOnce.toRoutes({
        '/redirectChain': (request, response) => {
          response.statusCode = 302;
          response.setHeader('Location', '/302');
          response.end();
        },
        '/302': (request, response) => {
          response.statusCode = 302;
          response.setHeader('Location', '/200');
          response.end();
        },
        '/200': (request, response) => {
          response.statusCode = 200;
          response.end();
        }
      });
      const urlRequest = net.request({ url: `${serverUrl}/redirectChain`, redirect: 'manual' });
      const redirects: string[] = [];
      urlRequest.on('redirect', (status, method, url) => {
        redirects.push(url);
        urlRequest.followRedirect();
      });
      const response = await getResponse(urlRequest);
      expect(response.statusCode).to.equal(200);
      expect(redirects).to.deep.equal([
        `${serverUrl}/302`,
        `${serverUrl}/200`
      ]);
    });

    it('should throw if given an invalid session option', () => {
      expect(() => {
        net.request({
          url: 'https://foo',
          session: 1 as any
        });
      }).to.throw('`session` should be an instance of the Session class');
    });

    it('should throw if given an invalid partition option', () => {
      expect(() => {
        net.request({
          url: 'https://foo',
          partition: 1 as any
        });
      }).to.throw('`partition` should be a string');
    });

    it('should be able to create a request with options', async () => {
      const customHeaderName = 'Some-Custom-Header-Name';
      const customHeaderValue = 'Some-Customer-Header-Value';
      const serverUrlUnparsed = await respondOnce.toURL('/', (request, response) => {
        expect(request.method).to.equal('GET');
        expect(request.headers[customHeaderName.toLowerCase()]).to.equal(customHeaderValue);
        response.statusCode = 200;
        response.statusMessage = 'OK';
        response.end();
      });
      const serverUrl = url.parse(serverUrlUnparsed);
      const options = {
        port: serverUrl.port ? parseInt(serverUrl.port, 10) : undefined,
        hostname: '127.0.0.1',
        headers: { [customHeaderName]: customHeaderValue }
      };
      const urlRequest = net.request(options);
      const response = await getResponse(urlRequest);
      expect(response.statusCode).to.be.equal(200);
      await collectStreamBody(response);
    });

    it('should be able to pipe a readable stream into a net request', async () => {
      const bodyData = randomString(kOneMegaByte);
      let netRequestReceived = false;
      let netRequestEnded = false;

      const [nodeServerUrl, netServerUrl] = await Promise.all([
        respondOnce.toSingleURL((request, response) => response.end(bodyData)),
        respondOnce.toSingleURL((request, response) => {
          netRequestReceived = true;
          let receivedBodyData = '';
          request.on('data', (chunk) => {
            receivedBodyData += chunk.toString();
          });
          request.on('end', (chunk: Buffer | undefined) => {
            netRequestEnded = true;
            if (chunk) {
              receivedBodyData += chunk.toString();
            }
            expect(receivedBodyData).to.be.equal(bodyData);
            response.end();
          });
        })
      ]);
      const nodeRequest = http.request(nodeServerUrl);
      const nodeResponse = await getResponse(nodeRequest as any) as any as http.ServerResponse;
      const netRequest = net.request(netServerUrl);
      const responsePromise = once(netRequest, 'response');
      // TODO(@MarshallOfSound) - FIXME with #22730
      nodeResponse.pipe(netRequest as any);
      const [netResponse] = await responsePromise;
      expect(netResponse.statusCode).to.equal(200);
      await collectStreamBody(netResponse);
      expect(netRequestReceived).to.be.true('net request received');
      expect(netRequestEnded).to.be.true('net request ended');
    });

    it('should report upload progress', async () => {
      const serverUrl = await respondOnce.toSingleURL((request, response) => {
        response.end();
      });
      const netRequest = net.request({ url: serverUrl, method: 'POST' });
      expect(netRequest.getUploadProgress()).to.have.property('active', false);
      netRequest.end(Buffer.from('hello'));
      const [position, total] = await once(netRequest, 'upload-progress');
      expect(netRequest.getUploadProgress()).to.deep.equal({ active: true, started: true, current: position, total });
    });

    it('should emit error event on server socket destroy', async () => {
      const serverUrl = await respondOnce.toSingleURL((request) => {
        request.socket.destroy();
      });
      const urlRequest = net.request(serverUrl);
      urlRequest.end();
      const [error] = await once(urlRequest, 'error');
      expect(error.message).to.equal('net::ERR_EMPTY_RESPONSE');
    });

    it('should emit error event on server request destroy', async () => {
      const serverUrl = await respondOnce.toSingleURL((request, response) => {
        request.destroy();
        response.end();
      });
      const urlRequest = net.request(serverUrl);
      urlRequest.end(randomBuffer(kOneMegaByte));
      const [error] = await once(urlRequest, 'error');
      expect(error.message).to.be.oneOf(['net::ERR_FAILED', 'net::ERR_CONNECTION_RESET', 'net::ERR_CONNECTION_ABORTED']);
    });

    it('should not emit any event after close', async () => {
      const serverUrl = await respondOnce.toSingleURL((request, response) => {
        response.end();
      });

      const urlRequest = net.request(serverUrl);
      urlRequest.end();

      await once(urlRequest, 'close');
      await new Promise((resolve, reject) => {
        for (const evName of ['finish', 'abort', 'close', 'error']) {
          urlRequest.on(evName as any, () => {
            reject(new Error(`Unexpected ${evName} event`));
          });
        }
        setTimeout(50).then(resolve);
      });
    });

    it('should remove the referer header when no referrer url specified', async () => {
      const serverUrl = await respondOnce.toSingleURL((request, response) => {
        expect(request.headers.referer).to.equal(undefined);
        response.statusCode = 200;
        response.statusMessage = 'OK';
        response.end();
      });
      const urlRequest = net.request(serverUrl);
      urlRequest.end();

      const response = await getResponse(urlRequest);
      expect(response.statusCode).to.equal(200);
      await collectStreamBody(response);
    });

    it('should set the referer header when a referrer url specified', async () => {
      const referrerURL = 'https://www.electronjs.org/';
      const serverUrl = await respondOnce.toSingleURL((request, response) => {
        expect(request.headers.referer).to.equal(referrerURL);
        response.statusCode = 200;
        response.statusMessage = 'OK';
        response.end();
      });
      // The referrerPolicy must be unsafe-url because the referrer's origin
      // doesn't match the loaded page. With the default referrer policy
      // (strict-origin-when-cross-origin), the request will be canceled by the
      // network service when the referrer header is invalid.
      // See:
      // - https://source.chromium.org/chromium/chromium/src/+/main:net/url_request/url_request.cc;l=682-683;drc=ae587fa7cd2e5cc308ce69353ee9ce86437e5d41
      // - https://source.chromium.org/chromium/chromium/src/+/main:services/network/public/mojom/network_context.mojom;l=316-318;drc=ae5c7fcf09509843c1145f544cce3a61874b9698
      // - https://w3c.github.io/webappsec-referrer-policy/#determine-requests-referrer
      const urlRequest = net.request({ url: serverUrl, referrerPolicy: 'unsafe-url' });
      urlRequest.setHeader('referer', referrerURL);
      urlRequest.end();

      const response = await getResponse(urlRequest);
      expect(response.statusCode).to.equal(200);
      await collectStreamBody(response);
    });
  });

  describe('IncomingMessage API', () => {
    it('response object should implement the IncomingMessage API', async () => {
      const customHeaderName = 'Some-Custom-Header-Name';
      const customHeaderValue = 'Some-Customer-Header-Value';

      const serverUrl = await respondOnce.toSingleURL((request, response) => {
        response.statusCode = 200;
        response.statusMessage = 'OK';
        response.setHeader(customHeaderName, customHeaderValue);
        response.end();
      });

      const urlRequest = net.request(serverUrl);
      const response = await getResponse(urlRequest);

      expect(response.statusCode).to.equal(200);
      expect(response.statusMessage).to.equal('OK');

      const headers = response.headers;
      expect(headers).to.be.an('object');
      const headerValue = headers[customHeaderName.toLowerCase()];
      expect(headerValue).to.equal(customHeaderValue);

      const rawHeaders = response.rawHeaders;
      expect(rawHeaders).to.be.an('array');
      expect(rawHeaders[0]).to.equal(customHeaderName);
      expect(rawHeaders[1]).to.equal(customHeaderValue);

      const httpVersion = response.httpVersion;
      expect(httpVersion).to.be.a('string').and.to.have.lengthOf.at.least(1);

      const httpVersionMajor = response.httpVersionMajor;
      expect(httpVersionMajor).to.be.a('number').and.to.be.at.least(1);

      const httpVersionMinor = response.httpVersionMinor;
      expect(httpVersionMinor).to.be.a('number').and.to.be.at.least(0);

      await collectStreamBody(response);
    });

    it('should discard duplicate headers', async () => {
      const includedHeader = 'max-forwards';
      const discardableHeader = 'Max-Forwards';

      const includedHeaderValue = 'max-fwds-val';
      const discardableHeaderValue = 'max-fwds-val-two';

      const serverUrl = await respondOnce.toSingleURL((request, response) => {
        response.statusCode = 200;
        response.statusMessage = 'OK';
        response.setHeader(discardableHeader, discardableHeaderValue);
        response.setHeader(includedHeader, includedHeaderValue);
        response.end();
      });
      const urlRequest = net.request(serverUrl);
      const response = await getResponse(urlRequest);
      expect(response.statusCode).to.equal(200);
      expect(response.statusMessage).to.equal('OK');

      const headers = response.headers;
      expect(headers).to.be.an('object');

      expect(headers).to.have.property(includedHeader);
      expect(headers).to.not.have.property(discardableHeader);
      expect(headers[includedHeader]).to.equal(includedHeaderValue);

      await collectStreamBody(response);
    });

    it('should join repeated non-discardable header values with ,', async () => {
      const serverUrl = await respondOnce.toSingleURL((request, response) => {
        response.statusCode = 200;
        response.statusMessage = 'OK';
        response.setHeader('referrer-policy', ['first-text', 'second-text']);
        response.end();
      });
      const urlRequest = net.request(serverUrl);
      const response = await getResponse(urlRequest);
      expect(response.statusCode).to.equal(200);
      expect(response.statusMessage).to.equal('OK');

      const headers = response.headers;
      expect(headers).to.be.an('object');
      expect(headers).to.have.property('referrer-policy');
      expect(headers['referrer-policy']).to.equal('first-text, second-text');

      await collectStreamBody(response);
    });

    it('should not join repeated discardable header values with ,', async () => {
      const serverUrl = await respondOnce.toSingleURL((request, response) => {
        response.statusCode = 200;
        response.statusMessage = 'OK';
        response.setHeader('last-modified', ['yesterday', 'today']);
        response.end();
      });
      const urlRequest = net.request(serverUrl);
      const response = await getResponse(urlRequest);
      expect(response.statusCode).to.equal(200);
      expect(response.statusMessage).to.equal('OK');

      const headers = response.headers;
      expect(headers).to.be.an('object');
      expect(headers).to.have.property('last-modified');
      expect(headers['last-modified']).to.equal('yesterday');

      await collectStreamBody(response);
    });

    it('should make set-cookie header an array even if single value', async () => {
      const serverUrl = await respondOnce.toSingleURL((request, response) => {
        response.statusCode = 200;
        response.statusMessage = 'OK';
        response.setHeader('set-cookie', 'chocolate-chip');
        response.end();
      });
      const urlRequest = net.request(serverUrl);
      const response = await getResponse(urlRequest);
      expect(response.statusCode).to.equal(200);
      expect(response.statusMessage).to.equal('OK');

      const headers = response.headers;
      expect(headers).to.be.an('object');
      expect(headers).to.have.property('set-cookie');
      expect(headers['set-cookie']).to.be.an('array');
      expect(headers['set-cookie'][0]).to.equal('chocolate-chip');

      await collectStreamBody(response);
    });

    it('should keep set-cookie header an array when an array', async () => {
      const serverUrl = await respondOnce.toSingleURL((request, response) => {
        response.statusCode = 200;
        response.statusMessage = 'OK';
        response.setHeader('set-cookie', ['chocolate-chip', 'oatmeal']);
        response.end();
      });
      const urlRequest = net.request(serverUrl);
      const response = await getResponse(urlRequest);
      expect(response.statusCode).to.equal(200);
      expect(response.statusMessage).to.equal('OK');

      const headers = response.headers;
      expect(headers).to.be.an('object');
      expect(headers).to.have.property('set-cookie');
      expect(headers['set-cookie']).to.be.an('array');
      expect(headers['set-cookie'][0]).to.equal('chocolate-chip');
      expect(headers['set-cookie'][1]).to.equal('oatmeal');

      await collectStreamBody(response);
    });

    it('should lowercase header keys', async () => {
      const serverUrl = await respondOnce.toSingleURL((request, response) => {
        response.statusCode = 200;
        response.statusMessage = 'OK';
        response.setHeader('HEADER-KEY', ['header-value']);
        response.setHeader('SeT-CookiE', ['chocolate-chip', 'oatmeal']);
        response.setHeader('rEFERREr-pOLICy', ['first-text', 'second-text']);
        response.setHeader('LAST-modified', 'yesterday');

        response.end();
      });
      const urlRequest = net.request(serverUrl);
      const response = await getResponse(urlRequest);
      expect(response.statusCode).to.equal(200);
      expect(response.statusMessage).to.equal('OK');

      const headers = response.headers;
      expect(headers).to.be.an('object');

      expect(headers).to.have.property('header-key');
      expect(headers).to.have.property('set-cookie');
      expect(headers).to.have.property('referrer-policy');
      expect(headers).to.have.property('last-modified');

      await collectStreamBody(response);
    });

    it('should return correct raw headers', async () => {
      const customHeaders: [string, string|string[]][] = [
        ['HEADER-KEY-ONE', 'header-value-one'],
        ['set-cookie', 'chocolate-chip'],
        ['header-key-two', 'header-value-two'],
        ['referrer-policy', ['first-text', 'second-text']],
        ['HEADER-KEY-THREE', 'header-value-three'],
        ['last-modified', ['first-text', 'second-text']],
        ['header-key-four', 'header-value-four']
      ];

      const serverUrl = await respondOnce.toSingleURL((request, response) => {
        response.statusCode = 200;
        response.statusMessage = 'OK';
        for (const headerTuple of customHeaders) {
          response.setHeader(headerTuple[0], headerTuple[1]);
        }
        response.end();
      });
      const urlRequest = net.request(serverUrl);
      const response = await getResponse(urlRequest);
      expect(response.statusCode).to.equal(200);
      expect(response.statusMessage).to.equal('OK');

      const rawHeaders = response.rawHeaders;
      expect(rawHeaders).to.be.an('array');

      let rawHeadersIdx = 0;
      for (const headerTuple of customHeaders) {
        const headerKey = headerTuple[0];
        const headerValues = Array.isArray(headerTuple[1]) ? headerTuple[1] : [headerTuple[1]];
        for (const headerValue of headerValues) {
          expect(rawHeaders[rawHeadersIdx]).to.equal(headerKey);
          expect(rawHeaders[rawHeadersIdx + 1]).to.equal(headerValue);
          rawHeadersIdx += 2;
        }
      }

      await collectStreamBody(response);
    });

    it('should be able to pipe a net response into a writable stream', async () => {
      const bodyData = randomString(kOneKiloByte);
      let nodeRequestProcessed = false;
      const [netServerUrl, nodeServerUrl] = await Promise.all([
        respondOnce.toSingleURL((request, response) => response.end(bodyData)),
        respondOnce.toSingleURL(async (request, response) => {
          const receivedBodyData = await collectStreamBody(request);
          expect(receivedBodyData).to.be.equal(bodyData);
          nodeRequestProcessed = true;
          response.end();
        })
      ]);
      const netRequest = net.request(netServerUrl);
      const netResponse = await getResponse(netRequest);
      const serverUrl = url.parse(nodeServerUrl);
      const nodeOptions = {
        method: 'POST',
        path: serverUrl.path,
        port: serverUrl.port
      };
      const nodeRequest = http.request(nodeOptions);
      const nodeResponsePromise = once(nodeRequest, 'response');
      // TODO(@MarshallOfSound) - FIXME with #22730
      (netResponse as any).pipe(nodeRequest);
      const [nodeResponse] = await nodeResponsePromise;
      netRequest.end();
      await collectStreamBody(nodeResponse);
      expect(nodeRequestProcessed).to.equal(true);
    });

    it('should correctly throttle an incoming stream', async () => {
      let numChunksSent = 0;
      const serverUrl = await respondOnce.toSingleURL((request, response) => {
        const data = randomString(kOneMegaByte);
        const write = () => {
          let ok = true;
          do {
            numChunksSent++;
            if (numChunksSent > 30) return;
            ok = response.write(data);
          } while (ok);
          response.once('drain', write);
        };
        write();
      });
      const urlRequest = net.request(serverUrl);
      urlRequest.on('response', () => {});
      urlRequest.end();
      await setTimeout(2000);
      // TODO(nornagon): I think this ought to max out at 20, but in practice
      // it seems to exceed that sometimes. This is at 25 to avoid test flakes,
      // but we should investigate if there's actually something broken here and
      // if so fix it and reset this to max at 20, and if not then delete this
      // comment.
      expect(numChunksSent).to.be.at.most(25);
    });
  });

  describe('net.isOnline', () => {
    it('getter returns boolean', () => {
      expect(net.isOnline()).to.be.a('boolean');
    });

    it('property returns boolean', () => {
      expect(net.online).to.be.a('boolean');
    });
  });

  describe('Stability and performance', () => {
    it('should free unreferenced, never-started request objects without crash', (done) => {
      net.request('https://test');
      process.nextTick(() => {
        const v8Util = process._linkedBinding('electron_common_v8_util');
        v8Util.requestGarbageCollectionForTesting();
        done();
      });
    });

    it('should collect on-going requests without crash', async () => {
      let finishResponse: (() => void) | null = null;
      const serverUrl = await respondOnce.toSingleURL((request, response) => {
        response.write(randomString(kOneKiloByte));
        finishResponse = () => {
          response.write(randomString(kOneKiloByte));
          response.end();
        };
      });
      const urlRequest = net.request(serverUrl);
      const response = await getResponse(urlRequest);
      process.nextTick(() => {
        // Trigger a garbage collection.
        const v8Util = process._linkedBinding('electron_common_v8_util');
        v8Util.requestGarbageCollectionForTesting();
        finishResponse!();
      });
      await collectStreamBody(response);
    });

    it('should collect unreferenced, ended requests without crash', async () => {
      const serverUrl = await respondOnce.toSingleURL((request, response) => {
        response.end();
      });
      const urlRequest = net.request(serverUrl);
      process.nextTick(() => {
        const v8Util = process._linkedBinding('electron_common_v8_util');
        v8Util.requestGarbageCollectionForTesting();
      });
      const response = await getResponse(urlRequest);
      await collectStreamBody(response);
    });

    it('should finish sending data when urlRequest is unreferenced', async () => {
      const serverUrl = await respondOnce.toSingleURL(async (request, response) => {
        const received = await collectStreamBodyBuffer(request);
        expect(received.length).to.equal(kOneMegaByte);
        response.end();
      });
      const urlRequest = net.request(serverUrl);
      urlRequest.on('close', () => {
        process.nextTick(() => {
          const v8Util = process._linkedBinding('electron_common_v8_util');
          v8Util.requestGarbageCollectionForTesting();
        });
      });
      urlRequest.write(randomBuffer(kOneMegaByte));
      const response = await getResponse(urlRequest);
      await collectStreamBody(response);
    });

    it('should finish sending data when urlRequest is unreferenced for chunked encoding', async () => {
      const serverUrl = await respondOnce.toSingleURL(async (request, response) => {
        const received = await collectStreamBodyBuffer(request);
        response.end();
        expect(received.length).to.equal(kOneMegaByte);
      });
      const urlRequest = net.request(serverUrl);
      urlRequest.chunkedEncoding = true;
      urlRequest.write(randomBuffer(kOneMegaByte));
      const response = await getResponse(urlRequest);
      await collectStreamBody(response);
      process.nextTick(() => {
        const v8Util = process._linkedBinding('electron_common_v8_util');
        v8Util.requestGarbageCollectionForTesting();
      });
    });

    it('should finish sending data when urlRequest is unreferenced before close event for chunked encoding', async () => {
      const serverUrl = await respondOnce.toSingleURL(async (request, response) => {
        const received = await collectStreamBodyBuffer(request);
        response.end();
        expect(received.length).to.equal(kOneMegaByte);
      });
      const urlRequest = net.request(serverUrl);
      urlRequest.chunkedEncoding = true;
      urlRequest.write(randomBuffer(kOneMegaByte));
      const v8Util = process._linkedBinding('electron_common_v8_util');
      v8Util.requestGarbageCollectionForTesting();
      await collectStreamBody(await getResponse(urlRequest));
    });

    it('should finish sending data when urlRequest is unreferenced', async () => {
      const serverUrl = await respondOnce.toSingleURL(async (request, response) => {
        const received = await collectStreamBodyBuffer(request);
        response.end();
        expect(received.length).to.equal(kOneMegaByte);
      });
      const urlRequest = net.request(serverUrl);
      urlRequest.on('close', () => {
        process.nextTick(() => {
          const v8Util = process._linkedBinding('electron_common_v8_util');
          v8Util.requestGarbageCollectionForTesting();
        });
      });
      urlRequest.write(randomBuffer(kOneMegaByte));
      await collectStreamBody(await getResponse(urlRequest));
    });

    it('should finish sending data when urlRequest is unreferenced for chunked encoding', async () => {
      const serverUrl = await respondOnce.toSingleURL(async (request, response) => {
        const received = await collectStreamBodyBuffer(request);
        response.end();
        expect(received.length).to.equal(kOneMegaByte);
      });
      const urlRequest = net.request(serverUrl);
      urlRequest.on('close', () => {
        process.nextTick(() => {
          const v8Util = process._linkedBinding('electron_common_v8_util');
          v8Util.requestGarbageCollectionForTesting();
        });
      });
      urlRequest.chunkedEncoding = true;
      urlRequest.write(randomBuffer(kOneMegaByte));
      await collectStreamBody(await getResponse(urlRequest));
    });
  });

  describe('non-http schemes', () => {
    it('should be rejected by net.request', async () => {
      expect(() => {
        net.request('file://bar');
      }).to.throw('ClientRequest only supports http: and https: protocols');
    });

    it('should be rejected by net.request when passed in url:', async () => {
      expect(() => {
        net.request({ url: 'file://bar' });
      }).to.throw('ClientRequest only supports http: and https: protocols');
    });
  });

  describe('net.fetch', () => {
    // NB. there exist much more comprehensive tests for fetch() in the form of
    // the WPT: https://github.com/web-platform-tests/wpt/tree/master/fetch
    // It's possible to run these tests against net.fetch(), but the test
    // harness to do so is quite complex and hasn't been munged to smoothly run
    // inside the Electron test runner yet.
    //
    // In the meantime, here are some tests for basic functionality and
    // Electron-specific behavior.

    describe('basic', () => {
      it('can fetch http urls', async () => {
        const serverUrl = await respondOnce.toSingleURL((request, response) => {
          response.end('test');
        });
        const resp = await net.fetch(serverUrl);
        expect(resp.ok).to.be.true();
        expect(await resp.text()).to.equal('test');
      });

      it('can upload a string body', async () => {
        const serverUrl = await respondOnce.toSingleURL((request, response) => {
          request.on('data', chunk => response.write(chunk));
          request.on('end', () => response.end());
        });
        const resp = await net.fetch(serverUrl, {
          method: 'POST',
          body: 'anchovies'
        });
        expect(await resp.text()).to.equal('anchovies');
      });

      it('can read response as an array buffer', async () => {
        const serverUrl = await respondOnce.toSingleURL((request, response) => {
          request.on('data', chunk => response.write(chunk));
          request.on('end', () => response.end());
        });
        const resp = await net.fetch(serverUrl, {
          method: 'POST',
          body: 'anchovies'
        });
        expect(new TextDecoder().decode(new Uint8Array(await resp.arrayBuffer()))).to.equal('anchovies');
      });

      it('can read response as form data', async () => {
        const serverUrl = await respondOnce.toSingleURL((request, response) => {
          response.setHeader('content-type', 'application/x-www-form-urlencoded');
          response.end('foo=bar');
        });
        const resp = await net.fetch(serverUrl);
        const result = await resp.formData();
        expect(result.get('foo')).to.equal('bar');
      });

      it('should be able to use a session cookie store', async () => {
        const serverUrl = await respondOnce.toSingleURL((request, response) => {
          response.statusCode = 200;
          response.statusMessage = 'OK';
          response.setHeader('x-cookie', request.headers.cookie!);
          response.end();
        });
        const sess = session.fromPartition(`cookie-tests-${Math.random()}`);
        const cookieVal = `${Date.now()}`;
        await sess.cookies.set({
          url: serverUrl,
          name: 'wild_cookie',
          value: cookieVal
        });
        const response = await sess.fetch(serverUrl, {
          credentials: 'include'
        });
        expect(response.headers.get('x-cookie')).to.equal(`wild_cookie=${cookieVal}`);
      });

      it('should reject promise on DNS failure', async () => {
        const r = net.fetch('https://i.do.not.exist');
        await expect(r).to.be.rejectedWith(/ERR_NAME_NOT_RESOLVED/);
      });

      it('should reject body promise when stream fails', async () => {
        const serverUrl = await respondOnce.toSingleURL((request, response) => {
          response.write('first chunk');
          setTimeout().then(() => response.destroy());
        });
        const r = await net.fetch(serverUrl);
        expect(r.status).to.equal(200);
        await expect(r.text()).to.be.rejectedWith(/ERR_INCOMPLETE_CHUNKED_ENCODING/);
      });
    });

    it('can request file:// URLs', async () => {
      const resp = await net.fetch(url.pathToFileURL(path.join(__dirname, 'fixtures', 'hello.txt')).toString());
      expect(resp.ok).to.be.true();
      // trimRight instead of asserting the whole string to avoid line ending shenanigans on WOA
      expect((await resp.text()).trimRight()).to.equal('hello world');
    });

    it('can make requests to custom protocols', async () => {
      protocol.registerStringProtocol('electron-test', (req, cb) => { cb('hello ' + req.url); });
      defer(() => {
        protocol.unregisterProtocol('electron-test');
      });
      const body = await net.fetch('electron-test://foo').then(r => r.text());
      expect(body).to.equal('hello electron-test://foo');
    });

    it('runs through intercept handlers', async () => {
      protocol.interceptStringProtocol('http', (req, cb) => { cb('hello ' + req.url); });
      defer(() => {
        protocol.uninterceptProtocol('http');
      });
      const body = await net.fetch('http://foo').then(r => r.text());
      expect(body).to.equal('hello http://foo/');
    });

    it('file: runs through intercept handlers', async () => {
      protocol.interceptStringProtocol('file', (req, cb) => { cb('hello ' + req.url); });
      defer(() => {
        protocol.uninterceptProtocol('file');
      });
      const body = await net.fetch('file://foo').then(r => r.text());
      expect(body).to.equal('hello file://foo/');
    });

    it('can be redirected', async () => {
      protocol.interceptStringProtocol('file', (req, cb) => { cb({ statusCode: 302, headers: { location: 'electron-test://bar' } }); });
      defer(() => {
        protocol.uninterceptProtocol('file');
      });
      protocol.registerStringProtocol('electron-test', (req, cb) => { cb('hello ' + req.url); });
      defer(() => {
        protocol.unregisterProtocol('electron-test');
      });
      const body = await net.fetch('file://foo').then(r => r.text());
      expect(body).to.equal('hello electron-test://bar');
    });

    it('should not follow redirect when redirect: error', async () => {
      protocol.registerStringProtocol('electron-test', (req, cb) => {
        if (/redirect/.test(req.url)) return cb({ statusCode: 302, headers: { location: 'electron-test://bar' } });
        cb('hello ' + req.url);
      });
      defer(() => {
        protocol.unregisterProtocol('electron-test');
      });
      await expect(net.fetch('electron-test://redirect', { redirect: 'error' })).to.eventually.be.rejectedWith('Attempted to redirect, but redirect policy was \'error\'');
    });

    it('a 307 redirected POST request preserves the body', async () => {
      const bodyData = 'Hello World!';
      let postedBodyData: any;
      protocol.registerStringProtocol('electron-test', async (req, cb) => {
        if (/redirect/.test(req.url)) return cb({ statusCode: 307, headers: { location: 'electron-test://bar' } });
        postedBodyData = req.uploadData![0].bytes.toString();
        cb('hello ' + req.url);
      });
      defer(() => {
        protocol.unregisterProtocol('electron-test');
      });
      const response = await net.fetch('electron-test://redirect', {
        method: 'POST',
        body: bodyData
      });
      expect(response.status).to.equal(200);
      await response.text();
      expect(postedBodyData).to.equal(bodyData);
    });
  });
});
