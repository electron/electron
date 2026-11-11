import { net, protocol, session } from 'electron/main';

import { describe, expect, it } from 'vitest';

import * as path from 'node:path';
import * as url from 'node:url';

import { defer } from './lib/spec-helpers.ts';

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

  describe('file: after an earlier request', () => {
    const fileUrl = url.pathToFileURL(path.join(import.meta.dirname, 'fixtures', 'hello.txt')).toString();
    // Range keeps these fetches on the URL loader, the path under test, even if plain file: GETs later bypass it.
    const fetchText = async (ses: Electron.Session) =>
      (await ses.fetch(fileUrl, { headers: { Range: 'bytes=0-' } }).then((r) => r.text())).trimEnd();

    it('runs through intercept handlers added since', async () => {
      const ses = session.fromPartition(`net-file-intercept-${Math.random()}`);
      expect(await fetchText(ses)).toBe('hello world');
      ses.protocol.interceptStringProtocol('file', (req, cb) => {
        cb('intercepted ' + req.url);
      });
      defer(() => {
        ses.protocol.uninterceptProtocol('file');
      });
      expect(await fetchText(ses)).toBe('intercepted ' + fileUrl);
      ses.protocol.uninterceptProtocol('file');
      expect(await fetchText(ses)).toBe('hello world');
    });

    it('triggers webRequest listeners added since, and not once removed', async () => {
      const ses = session.fromPartition(`net-file-webrequest-${Math.random()}`);
      await fetchText(ses);
      const seen: string[] = [];
      ses.webRequest.onBeforeRequest((details, cb) => {
        seen.push(details.url);
        cb({});
      });
      defer(() => {
        ses.webRequest.onBeforeRequest(null);
      });
      await fetchText(ses);
      expect(seen).toEqual([fileUrl]);
      ses.webRequest.onBeforeRequest(null);
      await fetchText(ses);
      expect(seen).toEqual([fileUrl]);
    });
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
});
