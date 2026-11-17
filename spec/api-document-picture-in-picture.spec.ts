import {
  app,
  BaseWindow,
  BrowserWindow,
  webContents,
  WebContentsView,
  type BrowserWindowConstructorOptions,
  type WebContents
} from 'electron/main';

import { afterAll, afterEach, beforeAll, beforeEach, describe, expect, it } from 'vitest';

import { once, type EventEmitter } from 'node:events';
import { readFileSync } from 'node:fs';
import * as http from 'node:http';
import * as path from 'node:path';

import { listen, waitUntil } from './lib/spec-helpers.ts';
import { closeAllWindows } from './lib/window-helpers.ts';

const requestScript = `documentPictureInPicture.requestWindow({ width: 360, height: 240 }).then(child => {
  window.pip = child;
  return true;
}, () => false)`;

describe('Document Picture-in-Picture hosting', () => {
  const server = http.createServer((request, response) => {
    response.setHeader('Content-Type', 'text/html');
    response.end(
      request.url === '/video'
        ? readFileSync(path.join(import.meta.dirname, 'fixtures/api/picture-in-picture.html'))
        : '<!doctype html><title>opener</title><button id="content">content</button>'
    );
  });
  let url: string;
  let opener: BrowserWindow;
  let children: WebContents[];
  let hosts: BaseWindow[];
  let dispositions: string[];

  beforeAll(async () => {
    url = (await listen(server)).url;
  });
  afterAll(() => new Promise<void>((resolve, reject) => server.close((error) => (error ? reject(error) : resolve()))));
  beforeEach(async () => {
    children = [];
    hosts = [];
    dispositions = [];
    opener = new BrowserWindow({ show: false, webPreferences: { sandbox: true, contextIsolation: true } });
    await opener.loadURL(`${url}/opener`);
  });
  afterEach(async () => {
    await closeAllWindows();
    for (const child of children) {
      if (!child.isDestroyed()) child.close({ waitForBeforeUnload: false });
    }
    await waitUntil(() => children.every((child) => child.isDestroyed()));
  });

  function host(options: BrowserWindowConstructorOptions, owner = opener, useView = false): WebContents {
    const child = (options as BrowserWindowConstructorOptions & { webContents: WebContents }).webContents;
    expect(options).toMatchObject({ width: 360, height: 240, useContentSize: true });
    if (!children.includes(child)) children.push(child);
    const presentation = {
      ...options,
      show: false,
      alwaysOnTop: true,
      fullscreenable: false,
      title: owner.webContents.mainFrame.origin
    };
    let window: BaseWindow;
    if (useView) {
      window = new BaseWindow(presentation);
      const view = new WebContentsView({ webContents: child });
      window.contentView.addChildView(view);
      view.setBounds({ x: 0, y: 0, width: 360, height: 240 });
    } else {
      window = new BrowserWindow(presentation);
    }
    window.setAlwaysOnTop(true, process.platform === 'win32' ? 'normal' : 'floating');
    hosts.push(window);
    window.once('closed', () => {
      if (!child.isDestroyed()) child.close({ waitForBeforeUnload: false });
    });
    child.once('destroyed', () => {
      if (!window.isDestroyed()) window.destroy();
    });
    return child;
  }

  function installHost(owner = opener, useView = false) {
    owner.webContents.setWindowOpenHandler((details) => {
      dispositions.push(details.disposition);
      return { action: 'allow', createWindow: (options) => host(options, owner, useView) };
    });
  }

  async function request(owner = opener, userGesture = true) {
    return owner.webContents.executeJavaScript(requestScript, userGesture);
  }

  async function open(useView = false) {
    installHost(opener, useView);
    expect(await request()).toBe(true);
    await waitUntil(() => children.length === 1);
    expect(dispositions).toEqual(['new-picture-in-picture']);
    expect(children).toHaveLength(1);
    return children[0];
  }

  it.each([false, true])(
    'preserves the real child Window and native PiP state (WebContentsView: %s)',
    async (useView) => {
      await opener.webContents.executeJavaScript(`window.entered = new Promise(resolve => {
      documentPictureInPicture.addEventListener('enter', event => resolve(event.window === documentPictureInPicture.window), { once: true });
    }); undefined`);
      const child = await open(useView);
      expect(
        await opener.webContents.executeJavaScript(`(async () => {
      const child = window.pip;
      const node = document.getElementById('content');
      window.clicks = 0;
      node.addEventListener('click', () => window.clicks++);
      child.document.body.append(node);
      node.click();
      return {
        identity: child === documentPictureInPicture.window,
        opener: child.opener === window,
        node: child.document.getElementById('content') === node,
        listener: window.clicks === 1,
        enter: await window.entered,
        mode: child.matchMedia('(display-mode: picture-in-picture)').matches,
        size: [child.innerWidth, child.innerHeight],
        nodeIntegration: typeof child.require
      };
    })()`)
      ).toEqual({
        identity: true,
        opener: true,
        node: true,
        listener: true,
        enter: true,
        mode: true,
        size: [360, 240],
        nodeIntegration: 'undefined'
      });
      expect(child.session).toBe(opener.webContents.session);
      expect(child.getLastWebPreferences()).toMatchObject({
        sandbox: true,
        contextIsolation: true,
        nodeIntegration: false
      });
      expect(hosts[0].isAlwaysOnTop()).toBe(true);
      expect(hosts[0].isFullScreenable()).toBe(false);
    }
  );

  it('fires pagehide and restores live content when the host closes', async () => {
    const child = await open();
    await opener.webContents.executeJavaScript(`window.restored = new Promise(resolve => {
      const node = document.getElementById('content');
      window.pip.document.body.append(node);
      window.pip.onbeforeunload = event => { event.preventDefault(); event.returnValue = "stay"; };
      window.pip.addEventListener('pagehide', () => {
        document.body.append(node);
        resolve(node === document.getElementById('content'));
      }, { once: true });
    }); undefined`);
    hosts[0].close();
    expect(await opener.webContents.executeJavaScript('window.restored')).toBe(true);
    await waitUntil(() => child.isDestroyed());
    expect(await opener.webContents.executeJavaScript('documentPictureInPicture.window === null')).toBe(true);
  });

  it('clears the native getter when the child closes and permits reopening', async () => {
    const first = await open();
    await opener.webContents.executeJavaScript('window.pip.close()');
    await waitUntil(() => first.isDestroyed());
    expect(await opener.webContents.executeJavaScript('documentPictureInPicture.window === null')).toBe(true);
    expect(await request()).toBe(true);
    await waitUntil(() => children.length === 2);
    expect(children).toHaveLength(2);
    expect(children[1].isDestroyed()).toBe(false);
  });

  it('replaces the previous document PiP without losing the new child', async () => {
    const first = await open();
    await opener.webContents.executeJavaScript('window.previous = window.pip; undefined');
    expect(await request()).toBe(true);
    await waitUntil(() => first.isDestroyed());
    expect(
      await opener.webContents.executeJavaScript(
        'window.previous.closed && window.pip !== window.previous && documentPictureInPicture.window === window.pip'
      )
    ).toBe(true);
    await waitUntil(() => children.length === 2);
    expect(children).toHaveLength(2);
    expect(children[1].isDestroyed()).toBe(false);
  });

  it('replaces PiP from a distinct opener without closing the new child', async () => {
    const first = await open();
    const other = new BrowserWindow({ show: false, webPreferences: { sandbox: true, contextIsolation: true } });
    await other.loadURL(`${url}/other`);
    installHost(other);
    expect(await request(other)).toBe(true);
    await waitUntil(() => first.isDestroyed() && children.length === 2);
    expect(await opener.webContents.executeJavaScript('documentPictureInPicture.window === null')).toBe(true);
    const second = children[1];
    expect(second.isDestroyed()).toBe(false);
    expect(await second.executeJavaScript('window.opener.documentPictureInPicture.window === window')).toBe(true);
    expect(
      await other.webContents.executeJavaScript('documentPictureInPicture.window === window.pip && !window.pip.closed')
    ).toBe(true);
  });

  it('survives same-document navigation', async () => {
    const child = await open();
    const navigated = once(opener.webContents, 'did-navigate-in-page');
    await opener.webContents.executeJavaScript('location.hash = "same-document"');
    await navigated;
    expect(child.isDestroyed()).toBe(false);
    expect(
      await opener.webContents.executeJavaScript('documentPictureInPicture.window === window.pip && !window.pip.closed')
    ).toBe(true);
  });

  it.each(['navigate', 'destroy'] as const)('closes when the opener is replaced (%s)', async (action) => {
    const child = await open();
    if (action === 'navigate') await opener.loadURL(`${url}/replacement`);
    else opener.destroy();
    await waitUntil(() => child.isDestroyed());
    expect(hosts[0].isDestroyed()).toBe(true);
  });

  it.each(['opener', 'child'] as const)(
    'cleans up after a renderer crash and permits reopening (%s)',
    async (target) => {
      const child = await open();
      const unrelated = new BrowserWindow({
        show: false,
        webPreferences: { sandbox: true, contextIsolation: true, partition: `document-pip-crash-${target}` }
      });
      await unrelated.loadURL('data:text/html,<title>unrelated</title>');
      const contents = target === 'opener' ? opener.webContents : child;
      expect(unrelated.webContents.getOSProcessId()).not.toBe(contents.getOSProcessId());
      const gone = once(contents, 'render-process-gone');
      contents.forcefullyCrashRenderer();
      const [, details] = await gone;
      // Windows/macOS can report killed after Shutdown(RESULT_CODE_HUNG).
      expect(['crashed', 'killed']).toContain(details.reason);
      await waitUntil(() => child.isDestroyed() && hosts[0].isDestroyed());
      expect(unrelated.isDestroyed()).toBe(false);
      expect(unrelated.webContents.isCrashed()).toBe(false);
      expect(await unrelated.webContents.executeJavaScript('document.title')).toBe('unrelated');
      expect(opener.isDestroyed()).toBe(false);
      // The child normally shares its renderer with the opener, so reload the
      // surviving opener WebContents for either crash target before reopening.
      const reloaded = once(opener.webContents, 'did-finish-load');
      opener.webContents.reload();
      await reloaded;
      expect(opener.webContents.isCrashed()).toBe(false);
      expect(await request()).toBe(true);
      await waitUntil(() => children.length === 2);
      expect(children[1].isDestroyed()).toBe(false);
      expect(
        await children[1].executeJavaScript(
          'window.opener.documentPictureInPicture.window === window && matchMedia("(display-mode: picture-in-picture)").matches'
        )
      ).toBe(true);
      expect(await unrelated.webContents.executeJavaScript('document.title')).toBe('unrelated');
    }
  );

  it('closes when the child starts a cross-document navigation', async () => {
    const child = await open();
    await opener.webContents.executeJavaScript(
      `window.pip.location.href = ${JSON.stringify(`${url}/child-navigation`)}`
    );
    await waitUntil(() => child.isDestroyed());
    expect(await opener.webContents.executeJavaScript('documentPictureInPicture.window === null')).toBe(true);
  });

  it.each(['no handler', 'deny', 'no host callback'])('rejects without an application host (%s)', async (policy) => {
    const before = webContents
      .getAllWebContents()
      .map((contents) => contents.id)
      .sort();
    if (policy !== 'no handler') {
      opener.webContents.setWindowOpenHandler(() => ({ action: policy === 'deny' ? 'deny' : 'allow' }));
    }
    expect(await request()).toBe(false);
    await waitUntil(
      () =>
        webContents
          .getAllWebContents()
          .map((contents) => contents.id)
          .sort()
          .join() === before.join()
    );
    expect(BaseWindow.getAllWindows()).toEqual([opener]);
  });

  it('requires transient user activation before calling the host', async () => {
    installHost();
    expect(await request(opener, false)).toBe(false);
    expect(dispositions).toEqual([]);
    expect(children).toEqual([]);
  });

  it.each(['child', 'opener'] as const)('tolerates destruction from the host callback (%s)', async (target) => {
    opener.webContents.setWindowOpenHandler(() => ({
      action: 'allow',
      createWindow(options) {
        const child = (options as BrowserWindowConstructorOptions & { webContents: WebContents }).webContents;
        children.push(child);
        if (target === 'child') child.close({ waitForBeforeUnload: false });
        else opener.destroy();
        return child;
      }
    }));
    // Destroying the opener may reject execution itself rather than the DOM promise.
    await request().catch(() => false);
    await waitUntil(() => children.length === 1 && children[0].isDestroyed());
    expect(hosts).toEqual([]);
    if (!opener.isDestroyed()) {
      await waitUntil(() => opener.webContents.executeJavaScript('documentPictureInPicture.window === null'));
      expect(await opener.webContents.executeJavaScript('documentPictureInPicture.window === null')).toBe(true);
    }
  });

  it.each(['throw', 'wrong child'] as const)('cleans up a failed host and permits reopening (%s)', async (failure) => {
    const before = webContents
      .getAllWebContents()
      .map((contents) => contents.id)
      .sort();
    const failureError = new Error('PiP host failed');
    let wrongWindow: BrowserWindow | undefined;
    opener.webContents.setWindowOpenHandler(() => ({
      action: 'allow',
      createWindow(options) {
        children.push((options as BrowserWindowConstructorOptions & { webContents: WebContents }).webContents);
        if (failure === 'throw') throw failureError;
        wrongWindow = new BrowserWindow({ show: false });
        return wrongWindow.webContents;
      }
    }));
    const listeners = process.listeners('uncaughtException');
    process.removeAllListeners('uncaughtException');
    let thrown: Error | undefined;
    process.on('uncaughtException', (error) => {
      thrown = error;
    });
    try {
      const requested = request().catch(() => false);
      await waitUntil(() => thrown !== undefined);
      if (failure === 'throw') expect(thrown).toBe(failureError);
      else {
        expect(thrown?.message).toBe(
          'Invalid webContents. Created window should be connected to webContents passed with options object.'
        );
      }
      await requested;
    } finally {
      process.removeAllListeners('uncaughtException');
      listeners.forEach((listener) => process.on('uncaughtException', listener));
    }
    await waitUntil(() => children.length === 1 && children[0].isDestroyed());
    if (wrongWindow) {
      // Failure cleanup owns the original guest, not the unrelated returned contents.
      expect(wrongWindow.isDestroyed()).toBe(false);
      wrongWindow.destroy();
    }
    await waitUntil(
      () =>
        webContents
          .getAllWebContents()
          .map((contents) => contents.id)
          .sort()
          .join() === before.join()
    );
    await waitUntil(() => opener.webContents.executeJavaScript('documentPictureInPicture.window === null'));
    expect(await opener.webContents.executeJavaScript('documentPictureInPicture.window === null')).toBe(true);
    installHost();
    expect(await request()).toBe(true);
    await waitUntil(() => children.length === 2);
    expect(children[1].isDestroyed()).toBe(false);
    expect(
      await children[1].executeJavaScript(
        'window.opener.documentPictureInPicture.window === window && matchMedia("(display-mode: picture-in-picture)").matches'
      )
    ).toBe(true);
  });

  it.each(['destroy', 'navigate'] as const)(
    'cleans up after an opener change from web-contents-created (%s)',
    async (action) => {
      installHost();
      const frame = opener.webContents.mainFrame;
      let navigation: Promise<void> | undefined;
      const onCreated = (_event: unknown, child: WebContents) => {
        if (child.opener !== frame) return;
        children.push(child);
        if (action === 'destroy') opener.destroy();
        else navigation = opener.loadURL(`${url}/creation-navigation`);
      };
      app.on('web-contents-created', onCreated);
      try {
        await request().catch(() => false);
        if (action === 'navigate') {
          expect(navigation).toBeDefined();
          // This asynchronous navigation tests normal creation-callback cleanup,
          // not a navigation commit from a nested native event loop.
          await navigation;
        }
        await waitUntil(() => children.length === 1 && children[0].isDestroyed());
        if (action === 'destroy') {
          expect(opener.isDestroyed()).toBe(true);
          expect(hosts).toEqual([]);
        } else {
          await waitUntil(() => hosts.every((window) => window.isDestroyed()));
        }
      } finally {
        app.removeListener('web-contents-created', onCreated);
      }
      if (action === 'navigate') {
        installHost();
        expect(await request()).toBe(true);
        await waitUntil(() => children.length === 2);
        expect(children[1].isDestroyed()).toBe(false);
        expect(await children[1].executeJavaScript('window.opener.documentPictureInPicture.window === window')).toBe(
          true
        );
      }
    }
  );

  it('tolerates destroying the opener from the native child close callback', async () => {
    const child = await open();
    let closed = false;
    (child as unknown as EventEmitter).once('close', () => {
      closed = true;
      opener.destroy();
    });
    await child.executeJavaScript('window.close()').catch(() => {});
    await waitUntil(() => closed && opener.isDestroyed() && child.isDestroyed());
  });

  it('permits a replacement request from the native child close callback', async () => {
    const child = await open();
    const other = new BrowserWindow({ show: false, webPreferences: { sandbox: true, contextIsolation: true } });
    await other.loadURL(`${url}/other`);
    installHost(other);
    let replacement: Promise<boolean> | undefined;
    (child as unknown as EventEmitter).once('close', () => {
      replacement = request(other);
    });
    await child.executeJavaScript('window.close()').catch(() => {});
    await waitUntil(() => replacement !== undefined);
    expect(await replacement).toBe(true);
    await waitUntil(() => child.isDestroyed());
    await waitUntil(() => children.length === 2);
    expect(children).toHaveLength(2);
    expect(children[1].isDestroyed()).toBe(false);
    expect(await other.webContents.executeJavaScript('documentPictureInPicture.window === window.pip')).toBe(true);
  });

  it('preserves native fullscreen and nested-request restrictions', async () => {
    const child = await open();
    expect(
      await child.executeJavaScript(
        `(async () => {
      const rejected = async operation => { try { await operation(); return false; } catch { return true; } };
      return [await rejected(() => documentPictureInPicture.requestWindow()), await rejected(() => document.body.requestFullscreen())];
    })()`,
        true
      )
    ).toEqual([true, true]);
    expect(children).toHaveLength(1);
  });

  it('replaces video PiP with document PiP and document PiP with video PiP', async () => {
    await opener.loadURL(`${url}/video`);
    await opener.webContents.executeJavaScript(`new Promise((resolve, reject) => {
      if (video.readyState >= 2) return resolve();
      video.addEventListener('loadeddata', () => resolve(), { once: true });
      video.addEventListener('error', () => reject(new Error('Video fixture failed to load')), { once: true });
    })`);
    expect(
      await opener.webContents.executeJavaScript(
        'video.requestPictureInPicture().then(() => document.pictureInPictureElement === video)',
        true
      )
    ).toBe(true);
    const child = await open();
    expect(await opener.webContents.executeJavaScript('document.pictureInPictureElement === null')).toBe(true);
    expect(
      await opener.webContents.executeJavaScript(
        'video.requestPictureInPicture().then(() => document.pictureInPictureElement === video)',
        true
      )
    ).toBe(true);
    await waitUntil(() => child.isDestroyed());
    expect(await opener.webContents.executeJavaScript('documentPictureInPicture.window === null')).toBe(true);
    await opener.webContents.executeJavaScript('document.exitPictureInPicture()');
  });
});
