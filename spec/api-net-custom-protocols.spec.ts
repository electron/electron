import { net, protocol, session } from 'electron/main';

import { afterAll, beforeAll, describe, expect, it } from 'vitest';

import { execFileSync } from 'node:child_process';
import * as fs from 'node:fs';
import * as originalFs from 'node:original-fs';
import * as os from 'node:os';
import * as path from 'node:path';
import * as url from 'node:url';

import { defer, ifit } from './lib/spec-helpers.ts';

describe('net module custom protocols', () => {
  it('can request file:// URLs', async () => {
    const resp = await net.fetch(url.pathToFileURL(path.join(import.meta.dirname, 'fixtures', 'hello.txt')).toString());
    expect(resp.ok).toBe(true);
    // trimRight instead of asserting the whole string to avoid line ending shenanigans on WOA
    expect((await resp.text()).trimRight()).toBe('hello world');
  });

  it('can make requests to custom protocols', async () => {
    protocol.registerStringProtocol('electron-test', (req, cb) => {
      cb('hello ' + req.url);
    });
    defer(() => {
      protocol.unregisterProtocol('electron-test');
    });
    const body = await net.fetch('electron-test://foo').then((r) => r.text());
    expect(body).toBe('hello electron-test://foo');
  });

  it('runs through intercept handlers', async () => {
    protocol.interceptStringProtocol('http', (req, cb) => {
      cb('hello ' + req.url);
    });
    defer(() => {
      protocol.uninterceptProtocol('http');
    });
    const body = await net.fetch('http://foo').then((r) => r.text());
    expect(body).toBe('hello http://foo/');
  });

  it('file: runs through intercept handlers', async () => {
    protocol.interceptStringProtocol('file', (req, cb) => {
      cb('hello ' + req.url);
    });
    defer(() => {
      protocol.uninterceptProtocol('file');
    });
    const body = await net.fetch('file://foo').then((r) => r.text());
    expect(body).toBe('hello file://foo/');
  });

  it('can be redirected', async () => {
    protocol.interceptStringProtocol('file', (req, cb) => {
      cb({ statusCode: 302, headers: { location: 'electron-test://bar' } });
    });
    defer(() => {
      protocol.uninterceptProtocol('file');
    });
    protocol.registerStringProtocol('electron-test', (req, cb) => {
      cb('hello ' + req.url);
    });
    defer(() => {
      protocol.unregisterProtocol('electron-test');
    });
    const body = await net.fetch('file://foo').then((r) => r.text());
    expect(body).toBe('hello electron-test://bar');
  });

  it('can be redirected from http to a custom scheme', async () => {
    protocol.interceptStringProtocol('http', (req, cb) => {
      cb({ statusCode: 302, headers: { location: 'electron-test://bar' } });
    });
    defer(() => {
      protocol.uninterceptProtocol('http');
    });
    protocol.registerStringProtocol('electron-test', (req, cb) => {
      cb('hello ' + req.url);
    });
    defer(() => {
      protocol.unregisterProtocol('electron-test');
    });
    const body = await net.fetch('http://foo').then((r) => r.text());
    expect(body).toBe('hello electron-test://bar');
  });

  it('can be redirected from file to file', async () => {
    protocol.interceptStringProtocol('file', (req, cb) => {
      if (/\/redirect-me$/.test(req.url)) return cb({ statusCode: 302, headers: { location: 'file:///target' } });
      cb('redirected-to ' + req.url);
    });
    defer(() => {
      protocol.uninterceptProtocol('file');
    });
    const body = await net.fetch('file:///redirect-me').then((r) => r.text());
    expect(body).toBe('redirected-to file:///target');
  });

  it('should not follow redirect when redirect: error', async () => {
    protocol.registerStringProtocol('electron-test', (req, cb) => {
      if (/redirect/.test(req.url)) return cb({ statusCode: 302, headers: { location: 'electron-test://bar' } });
      cb('hello ' + req.url);
    });
    defer(() => {
      protocol.unregisterProtocol('electron-test');
    });
    await expect(net.fetch('electron-test://redirect', { redirect: 'error' })).rejects.toThrow(
      "Attempted to redirect, but redirect policy was 'error'"
    );
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
    expect(response.status).toBe(200);
    await response.text();
    expect(postedBodyData).toBe(bodyData);
  });

  // net.fetch() answers these from the file, without a URL loader request,
  // and must not be distinguishable from the loader's answer.
  describe('file: URLs answered from the file', () => {
    const big = Buffer.alloc(300 * 1024, 'x');
    let dir: string;
    let fifoWriter: number | undefined;
    const fileUrl = (name: string) => url.pathToFileURL(path.join(dir, name)).toString();
    const newSession = () => session.fromPartition(`net-file-${Math.random()}`);
    // A Range header sends the fetch to the URL loader; for bytes=0- it
    // returns the whole file with the same status and headers.
    const viaLoader = (ses: Electron.Session, target: string, init: RequestInit = {}) =>
      ses.fetch(target, { ...init, headers: { ...(init.headers as Record<string, string>), Range: 'bytes=0-' } });
    const describeResponse = async (res: Response) => ({
      status: res.status,
      statusText: res.statusText,
      headers: [...res.headers],
      body: Buffer.from(await res.arrayBuffer()).toString('base64')
    });

    beforeAll(() => {
      dir = fs.mkdtempSync(path.join(os.tmpdir(), 'electron-net-file-'));
      fs.writeFileSync(path.join(dir, 'a.js'), 'export const a = 1;\n');
      fs.writeFileSync(path.join(dir, 'b.css'), 'body {}\n');
      fs.writeFileSync(path.join(dir, 'c.unknownext'), '<html><body>x</body></html>');
      fs.writeFileSync(path.join(dir, 'big.bin'), big);
      fs.writeFileSync(path.join(dir, 'empty.txt'), '');
      fs.mkdirSync(path.join(dir, 'sub'));
    });
    afterAll(() => {
      if (fifoWriter !== undefined) fs.closeSync(fifoWriter);
      fs.rmSync(dir, { recursive: true, force: true });
    });

    it('returns what the URL loader returns', async () => {
      const ses = newSession();
      for (const name of ['a.js', 'b.css', 'c.unknownext', 'big.bin']) {
        const direct = await describeResponse(await ses.fetch(fileUrl(name)));
        expect(direct, name).toEqual(await describeResponse(await viaLoader(ses, fileUrl(name))));
      }
    });

    it('answers HEAD and empty files like the URL loader', async () => {
      const ses = newSession();
      const head = await ses.fetch(fileUrl('a.js'), { method: 'HEAD' });
      const get = await ses.fetch(fileUrl('a.js'));
      expect(head.status).toBe(200);
      expect([...head.headers]).toEqual([...get.headers]);
      expect(head.body).toBeNull();
      const empty = await ses.fetch(fileUrl('empty.txt'));
      expect(answeredByLoader(empty)).toBe(false);
      expect(empty.status).toBe(200);
      expect(empty.headers.get('content-type')).toBe('text/plain');
      expect(await empty.text()).toBe('');
    });

    it('rejects like the URL loader', async () => {
      const ses = newSession();
      await expect(ses.fetch(fileUrl('missing.js'))).rejects.toThrow('net::ERR_FILE_NOT_FOUND');
      await expect(ses.fetch(fileUrl('sub'))).rejects.toThrow('net::ERR_FILE_NOT_FOUND');
    });

    ifit(process.platform !== 'win32' && process.getuid?.() !== 0)(
      'rejects an unreadable file like the URL loader',
      async () => {
        const locked = path.join(dir, 'locked.txt');
        fs.writeFileSync(locked, 'secret');
        fs.chmodSync(locked, 0);
        defer(() => {
          fs.chmodSync(locked, 0o600);
        });
        await expect(newSession().fetch(url.pathToFileURL(locked).toString())).rejects.toThrow(
          'net::ERR_ACCESS_DENIED'
        );
      }
    );

    // Nothing a caller can see tells the two answers apart, which is the
    // point; the URL loader's Response carries its loader for
    // protocol.handle(), and that is how these tests know which one answered.
    const answeredByLoader = (res: Response) => (res as any).__fetch !== undefined;

    it('answers without a URL loader request', async () => {
      const ses = newSession();
      expect(answeredByLoader(await ses.fetch(fileUrl('a.js')))).toBe(false);
      expect(answeredByLoader(await viaLoader(ses, fileUrl('a.js')))).toBe(true);
    });

    // The loader completes the request for a larger file only as its body is
    // read, so those keep going to it.
    it("leaves a file larger than the URL loader's data pipe to it", async () => {
      const name = path.join(dir, 'huge.bin');
      fs.writeFileSync(name, Buffer.alloc(8 * 1024 * 1024, 'y'));
      const res = await newSession().fetch(url.pathToFileURL(name).toString());
      expect(answeredByLoader(res)).toBe(true);
      expect((await res.arrayBuffer()).byteLength).toBe(8 * 1024 * 1024);
    });

    it('returns the bytes of an asar archive like the URL loader', async () => {
      const archive = path.join(import.meta.dirname, 'fixtures', 'test.asar', 'a.asar');
      const res = await newSession().fetch(url.pathToFileURL(archive).toString());
      expect(Buffer.from(await res.arrayBuffer())).toEqual(originalFs.readFileSync(archive));
    });

    ifit(process.platform === 'linux')('reads a file whose name is not UTF-8', async () => {
      const name = Buffer.concat([Buffer.from(path.join(dir, 'caf')), Buffer.from([0xe9]), Buffer.from('.txt')]);
      fs.writeFileSync(name, 'latin1');
      const res = await newSession().fetch(`${url.pathToFileURL(dir)}/caf%E9.txt`);
      expect(answeredByLoader(res)).toBe(false);
      expect(await res.text()).toBe('latin1');
    });

    // The loader has the file open, and one this size in its pipe, before the
    // fetch resolves.
    it('returns the bytes the file had when the fetch resolved', async () => {
      const ses = newSession();
      const deleted = path.join(dir, 'deleted.txt');
      fs.writeFileSync(deleted, 'before');
      const res = await ses.fetch(url.pathToFileURL(deleted).toString());
      expect(answeredByLoader(res)).toBe(false);
      fs.rmSync(deleted);
      expect(await res.text()).toBe('before');

      const replaced = path.join(dir, 'replaced.txt');
      fs.writeFileSync(replaced, 'before');
      const res2 = await ses.fetch(url.pathToFileURL(replaced).toString());
      fs.writeFileSync(`${replaced}.new`, 'after!');
      fs.renameSync(`${replaced}.new`, replaced);
      expect(await res2.text()).toBe('before');
    });

    it('rejects with the abort reason when aborted before the response', async () => {
      const controller = new AbortController();
      const response = newSession().fetch(fileUrl('a.js'), { signal: controller.signal });
      controller.abort(new Error('stopped'));
      await expect(response).rejects.toThrow('stopped');
    });

    // Opening a FIFO waits for a writer, as opening a file on a hung mount can.
    ifit(process.platform !== 'win32')('rejects at once when aborted while opening the file blocks', async () => {
      const fifo = path.join(dir, 'fifo');
      execFileSync('mkfifo', [fifo]);
      // Lets the open finish whenever it starts. Closed in afterAll().
      defer(() => {
        fifoWriter = fs.openSync(fifo, 'r+');
      });
      const controller = new AbortController();
      const response = newSession().fetch(url.pathToFileURL(fifo).toString(), { signal: controller.signal });
      controller.abort(new Error('stopped'));
      await expect(response).rejects.toThrow('stopped');
    });

    it('errors the body when aborted while it is read', async () => {
      const controller = new AbortController();
      const res = await newSession().fetch(fileUrl('big.bin'), { signal: controller.signal });
      controller.abort();
      await expect(res.arrayBuffer()).rejects.toThrow();
    });

    describe('webRequest', () => {
      type Recorded = { event: string; details: Record<string, any> };
      // Collects events, dropping what differs between any two requests and
      // the Range header that sends a fetch to the URL loader.
      const collector = (events: Recorded[]) => (event: string) => (details: any) => {
        const { id, timestamp, requestHeaders, ...rest } = details;
        if (requestHeaders) {
          const { range, ...headers } = requestHeaders;
          rest.requestHeaders = headers;
        }
        events.push({ event, details: rest });
      };
      const record = (ses: Electron.Session) => {
        const events: Recorded[] = [];
        const add = collector(events);
        ses.webRequest.onBeforeRequest((details, callback) => {
          add('onBeforeRequest')(details);
          callback({});
        });
        ses.webRequest.onBeforeSendHeaders((details, callback) => {
          add('onBeforeSendHeaders')(details);
          callback({});
        });
        ses.webRequest.onSendHeaders(add('onSendHeaders'));
        ses.webRequest.onHeadersReceived((details, callback) => {
          add('onHeadersReceived')(details);
          callback({});
        });
        ses.webRequest.onBeforeRedirect(add('onBeforeRedirect'));
        ses.webRequest.onResponseStarted(add('onResponseStarted'));
        ses.webRequest.onCompleted(add('onCompleted'));
        ses.webRequest.onErrorOccurred(add('onErrorOccurred'));
        return events;
      };
      const settled = async (events: Recorded[], count: number) => {
        for (let i = 0; i < 100 && events.length < count; i++) await new Promise((resolve) => setTimeout(resolve, 10));
        return [...events];
      };

      it('sees the events and details the URL loader request has', async () => {
        const ses = newSession();
        const events = record(ses);
        await (await ses.fetch(fileUrl('a.js'))).text();
        const direct = await settled(events, 3);
        events.length = 0;
        await (await viaLoader(ses, fileUrl('a.js'))).text();
        const loader = await settled(events, 3);
        expect(direct.map((e) => e.event)).toEqual(['onBeforeRequest', 'onResponseStarted', 'onCompleted']);
        expect(direct).toEqual(loader);
      });

      it('sees the request its headers describe, as on the URL loader request', async () => {
        const ses = newSession();
        const events = record(ses);
        const init = {
          headers: { 'Sec-Fetch-Dest': 'script', Referer: 'https://referrer.test/', Origin: 'https://origin.test' }
        };
        await (await ses.fetch(fileUrl('a.js'), init)).text();
        const direct = await settled(events, 3);
        events.length = 0;
        await (await viaLoader(ses, fileUrl('a.js'), init)).text();
        const loader = await settled(events, 3);
        expect(direct[0].details).toMatchObject({
          resourceType: 'script',
          referrer: 'https://referrer.test/',
          initiatorOrigin: 'https://origin.test'
        });
        expect(direct).toEqual(loader);
      });

      // With no blocking listener, the loader's request is only observed, and
      // observers hear about its headers.
      it('tells observers what the URL loader request tells them', async () => {
        const ses = newSession();
        const events: Recorded[] = [];
        const add = collector(events);
        ses.webRequest.onSendHeaders(add('onSendHeaders'));
        ses.webRequest.onResponseStarted(add('onResponseStarted'));
        ses.webRequest.onCompleted(add('onCompleted'));
        const init = { headers: { 'Sec-Fetch-Dest': 'style', Origin: 'https://origin.test' } };
        await (await ses.fetch(fileUrl('b.css'), init)).text();
        const direct = await settled(events, 3);
        events.length = 0;
        await (await viaLoader(ses, fileUrl('b.css'), init)).text();
        const loader = await settled(events, 3);
        expect(direct.map((e) => e.event)).toEqual(['onSendHeaders', 'onResponseStarted', 'onCompleted']);
        expect(direct[0].details.requestHeaders).toEqual({
          'Sec-Fetch-Mode': 'cors',
          origin: 'https://origin.test',
          'sec-fetch-dest': 'style'
        });
        expect(direct).toEqual(loader);
      });

      it('completes the request before the body is read, as the URL loader does', async () => {
        const ses = newSession();
        const completed = new Promise((resolve) => ses.webRequest.onCompleted(resolve));
        const res = await ses.fetch(fileUrl('big.bin'));
        await completed;
        expect((await res.arrayBuffer()).byteLength).toBe(big.length);
      });

      it('rejects when a listener cancels', async () => {
        const ses = newSession();
        const errors: string[] = [];
        ses.webRequest.onBeforeRequest((details, callback) => callback({ cancel: true }));
        ses.webRequest.onErrorOccurred((details) => errors.push(details.error));
        await expect(ses.fetch(fileUrl('a.js'))).rejects.toThrow('net::ERR_BLOCKED_BY_CLIENT');
        expect(errors).toEqual(['net::ERR_BLOCKED_BY_CLIENT']);
      });

      it("follows a listener's redirect as a new request", async () => {
        const ses = newSession();
        const events: { event: string; id: number; url: string; status?: number; error?: string }[] = [];
        ses.webRequest.onBeforeRequest((details, callback) => {
          events.push({ event: 'onBeforeRequest', id: details.id, url: details.url });
          callback(details.url === fileUrl('a.js') ? { redirectURL: fileUrl('b.css') } : {});
        });
        ses.webRequest.onBeforeRedirect((details) => {
          events.push({ event: 'onBeforeRedirect', id: details.id, url: details.url, status: details.statusCode });
        });
        ses.webRequest.onErrorOccurred((details) => {
          events.push({ event: 'onErrorOccurred', id: details.id, url: details.url, error: details.error });
        });
        expect(await (await ses.fetch(fileUrl('a.js'))).text()).toBe('body {}\n');
        expect(events.map(({ event, url, status, error }) => ({ event, url, status, error }))).toEqual([
          { event: 'onBeforeRequest', url: fileUrl('a.js'), status: undefined, error: undefined },
          { event: 'onBeforeRedirect', url: fileUrl('a.js'), status: 307, error: undefined },
          { event: 'onErrorOccurred', url: fileUrl('a.js'), status: undefined, error: 'net::ERR_ABORTED' },
          { event: 'onBeforeRequest', url: fileUrl('b.css'), status: undefined, error: undefined }
        ]);
        expect(events[3].id).not.toBe(events[0].id);
      });

      it("stops at a listener's redirect when redirect is 'error' or 'manual'", async () => {
        const ses = newSession();
        ses.webRequest.onBeforeRequest((details, callback) => {
          callback(details.url === fileUrl('a.js') ? { redirectURL: fileUrl('b.css') } : {});
        });
        await expect(ses.fetch(fileUrl('a.js'), { redirect: 'error' })).rejects.toThrow(
          "Attempted to redirect, but redirect policy was 'error'"
        );
        await expect(ses.fetch(fileUrl('a.js'), { redirect: 'manual' })).rejects.toThrow('Redirect was cancelled');
      });

      it('rejects a redirect the URL loader would not follow', async () => {
        const ses = newSession();
        const events: string[] = [];
        ses.webRequest.onBeforeRequest((details, callback) => {
          events.push('onBeforeRequest');
          callback({ redirectURL: 'data:text/plain,x' });
        });
        ses.webRequest.onBeforeRedirect((details) => events.push(`onBeforeRedirect ${details.redirectURL}`));
        ses.webRequest.onErrorOccurred((details) => events.push(`onErrorOccurred ${details.error}`));
        await expect(ses.fetch(fileUrl('a.js'))).rejects.toThrow('net::ERR_UNSAFE_REDIRECT');
        expect(events).toEqual([
          'onBeforeRequest',
          'onBeforeRedirect data:text/plain,x',
          'onErrorOccurred net::ERR_ABORTED'
        ]);
      });

      it('rejects when a listener aborts the fetch', async () => {
        const ses = newSession();
        const controller = new AbortController();
        ses.webRequest.onBeforeRequest((details, callback) => callback({}));
        ses.webRequest.onResponseStarted(() => controller.abort(new Error('stopped by a listener')));
        await expect(ses.fetch(fileUrl('a.js'), { signal: controller.signal })).rejects.toThrow(
          'stopped by a listener'
        );
      });

      it('rejects when a listener aborts the fetch from a microtask', async () => {
        const ses = newSession();
        const controller = new AbortController();
        ses.webRequest.onBeforeRequest((details, callback) => callback({}));
        ses.webRequest.onResponseStarted(() => queueMicrotask(() => controller.abort(new Error('stopped later'))));
        await expect(ses.fetch(fileUrl('a.js'), { signal: controller.signal })).rejects.toThrow('stopped later');
      });

      it('skips listeners whose filter excludes the URL', async () => {
        const ses = newSession();
        const seen: string[] = [];
        ses.webRequest.onBeforeRequest({ urls: ['<all_urls>'], excludeUrls: ['file:///*'] }, (details, callback) => {
          seen.push(details.url);
          callback({});
        });
        expect(await (await ses.fetch(fileUrl('a.js'))).text()).toBe('export const a = 1;\n');
        expect(seen).toEqual([]);
      });
    });

    it('uses a handler registered for file:', async () => {
      const ses = newSession();
      ses.protocol.registerFileProtocol('file', (request, callback) => callback({ path: path.join(dir, 'b.css') }));
      defer(() => {
        ses.protocol.unregisterProtocol('file');
      });
      expect(await (await ses.fetch(fileUrl('a.js'))).text()).toBe('body {}\n');
    });
  });
});
