import { BaseWindow, BrowserWindow, View, WebContentsView, webContents, screen } from 'electron/main';

import { afterEach, beforeEach, describe, expect, it } from 'vitest';

import { once } from 'node:events';
import { setTimeout as setTimeoutAsync } from 'node:timers/promises';

import { HexColors, ScreenCapture, hasCapturableScreen, nextFrameTime } from './lib/screen-helpers.ts';
import { defer, ifdescribe, waitUntil } from './lib/spec-helpers.ts';
import { closeAllWindows } from './lib/window-helpers.ts';

describe('WebContentsView', () => {
  afterEach(async () => {
    await closeAllWindows();
    const existingWCS = webContents.getAllWebContents();
    existingWCS.forEach((contents) => contents.close());
  });

  it('can be instantiated with no arguments', () => {
    // oxlint-disable-next-line no-new
    new WebContentsView();
  });

  it('can be instantiated with no webPreferences', () => {
    // oxlint-disable-next-line no-new
    new WebContentsView({});
  });

  it('accepts existing webContents object', async () => {
    // Compare against the set rather than a count: webContents left to the
    // garbage collector by earlier tests may go away while this one runs.
    const existingWebContents = new Set(webContents.getAllWebContents());

    const wc = (webContents as typeof ElectronInternal.WebContents).create({ sandbox: true });
    defer(() => wc.destroy());
    await wc.loadURL('about:blank');

    const webContentsView = new WebContentsView({
      webContents: wc
    });

    expect(webContentsView.webContents).toBe(wc);
    expect(
      webContents.getAllWebContents().filter((contents) => !existingWebContents.has(contents)),
      'expected only single webcontents to be created'
    ).toEqual([wc]);
  });

  it('ignores native updates after an externally owned webContents is destroyed', async () => {
    const internalWebContents = webContents as typeof ElectronInternal.WebContents;
    const embedder = internalWebContents.create();
    defer(() => embedder.destroy());
    await embedder.loadURL('data:text/html,<iframe src="about:blank"></iframe>');

    const guest = internalWebContents.create({
      type: 'webview',
      embedder
    });
    const webContentsView = new WebContentsView({ webContents: guest });
    await guest.loadURL('about:blank');
    guest.attachToIframe(embedder, embedder.mainFrame.frames[0].frameToken);

    const destroyed = once(guest, 'destroyed');
    guest.destroy();
    await destroyed;

    expect(webContentsView.webContents).toBe(guest);
    expect(() => webContentsView.setBackgroundColor('#123456')).not.toThrow();
    expect(() => webContentsView.setBorderRadius(4)).not.toThrow();
  });

  it('should throw error when created with already attached webContents to BrowserWindow', () => {
    const browserWindow = new BrowserWindow();
    defer(() => browserWindow.webContents.destroy());

    const webContentsView = new WebContentsView();
    defer(() => webContentsView.webContents.destroy());

    browserWindow.contentView.addChildView(webContentsView);
    defer(() => browserWindow.contentView.removeChildView(webContentsView));

    expect(
      () =>
        new WebContentsView({
          webContents: webContentsView.webContents
        })
    ).toThrow('options.webContents is already attached to a window');
  });

  it('should throw an error when adding a destroyed child view to the parent view', async () => {
    const browserWindow = new BrowserWindow();

    const webContentsView = new WebContentsView();
    const wc = webContentsView.webContents;
    wc.loadURL('about:blank');
    wc.destroy();

    const destroyed = once(wc, 'destroyed');
    await destroyed;
    expect(() => browserWindow.contentView.addChildView(webContentsView)).toThrow(
      "Can't add a destroyed child view to a parent view"
    );
  });

  it('should throw error when created with already attached webContents to other WebContentsView', () => {
    const browserWindow = new BrowserWindow();

    const webContentsView = new WebContentsView();
    defer(() => webContentsView.webContents.destroy());
    webContentsView.webContents.loadURL('about:blank');

    expect(
      () =>
        new WebContentsView({
          webContents: browserWindow.webContents
        })
    ).toThrow('options.webContents is already attached to a window');
  });

  it('can be used as content view', () => {
    const w = new BaseWindow({ show: false });
    w.setContentView(new WebContentsView());
  });

  it('can be removed after a close', async () => {
    const w = new BaseWindow({ show: false });
    const v = new View();
    const wcv = new WebContentsView();
    const wc = wcv.webContents;
    w.setContentView(v);
    v.addChildView(wcv);
    await wc.loadURL('about:blank');
    const destroyed = once(wc, 'destroyed');
    wc.executeJavaScript('window.close()');
    await destroyed;
    expect(wc.isDestroyed()).toBe(true);
    v.removeChildView(wcv);
  });

  it('leaves its parent and stops responding once its webContents is destroyed', async () => {
    const w = new BaseWindow({ show: false });
    const v = new View();
    const wcv = new WebContentsView();
    const child = new View();
    const wc = wcv.webContents;
    w.setContentView(v);
    v.addChildView(wcv);
    wcv.addChildView(child);
    wcv.setBounds({ x: 1, y: 2, width: 30, height: 40 });
    expect(wcv.children).toEqual([child]);
    expect(wcv.getVisible()).toBe(true);

    const destroyed = once(wc, 'destroyed');
    wc.destroy();
    await destroyed;

    expect(v.children).toEqual([]);
    expect(wcv.getBounds()).toEqual({ x: 0, y: 0, width: 0, height: 0 });
    expect(wcv.getVisible()).toBe(false);
    wcv.setBounds({ x: 5, y: 5, width: 5, height: 5 });
    expect(wcv.getBounds()).toEqual({ x: 0, y: 0, width: 0, height: 0 });
    wcv.addChildView(new View());
    expect(() => v.addChildView(wcv)).toThrow("Can't add a destroyed child view to a parent view");
  });

  it('correctly reorders children', () => {
    const w = new BaseWindow({ show: false });
    const cv = new View();
    w.setContentView(cv);

    const wcv1 = new WebContentsView();
    const wcv2 = new WebContentsView();
    const wcv3 = new WebContentsView();
    w.contentView.addChildView(wcv1);
    w.contentView.addChildView(wcv2);
    w.contentView.addChildView(wcv3);

    expect(w.contentView.children).toEqual([wcv1, wcv2, wcv3]);

    w.contentView.addChildView(wcv1);
    w.contentView.addChildView(wcv2);
    expect(w.contentView.children).toEqual([wcv3, wcv1, wcv2]);
  });

  it('handle removal and re-addition of children', () => {
    const w = new BaseWindow({ show: false });
    const cv = new View();
    w.setContentView(cv);

    const wcv1 = new WebContentsView();
    const wcv2 = new WebContentsView();

    expect(w.contentView.children).toEqual([]);

    w.contentView.addChildView(wcv1);
    w.contentView.addChildView(wcv2);

    expect(w.contentView.children).toEqual([wcv1, wcv2]);

    w.contentView.removeChildView(wcv1);

    expect(w.contentView.children).toEqual([wcv2]);

    w.contentView.addChildView(wcv1);

    expect(w.contentView.children).toEqual([wcv2, wcv1]);
  });

  function triggerGCByAllocation() {
    const arr = [];
    for (let i = 0; i < 1000000; i++) {
      arr.push([]);
    }
    return arr;
  }

  it("doesn't crash when GCed during allocation", () =>
    new Promise<void>((resolve) => {
      // oxlint-disable-next-line no-new
      new WebContentsView();
      setTimeout(() => {
        // NB. the crash we're testing for is the lack of a current `v8::Context`
        // when emitting an event in WebContents's destructor. V8 is inconsistent
        // about whether or not there's a current context during garbage
        // collection, and it seems that `v8Util.requestGarbageCollectionForTesting`
        // causes a GC in which there _is_ a current context, so the crash isn't
        // triggered. Thus, we force a GC by other means: namely, by allocating a
        // bunch of stuff.
        triggerGCByAllocation();
        resolve();
      });
    }));

  // Waits for 'blur', which needs the view to have had focus; a window that
  // isn't active doesn't give its new contents initial focus.
  it('does not crash when closed via window.close()', { tags: ['serial'] }, async () => {
    const bw = new BrowserWindow();
    const wcv = new WebContentsView();
    const wc = wcv.webContents;

    await bw.loadURL('data:text/html,<h1>Main Window</h1>');
    bw.contentView.addChildView(wcv);

    const dto = new Promise<boolean>((resolve) => {
      wc.on('blur', () => {
        const devToolsOpen = !wc.isDestroyed() && wc.isDevToolsOpened();
        resolve(devToolsOpen);
      });
    });

    wc.loadURL('data:text/html,<script>window.close()</script>');

    const open = await dto;
    expect(open).toBe(false);
  });

  it('can be fullscreened', { tags: ['serial'] }, async () => {
    const w = new BaseWindow();
    const v = new WebContentsView();
    w.setContentView(v);
    await v.webContents.loadURL('data:text/html,<div id="div">This is a simple div.</div>');

    const enterFullScreen = once(w, 'enter-full-screen');
    await v.webContents.executeJavaScript('document.getElementById("div").requestFullscreen()', true);
    await enterFullScreen;
    expect(w.isFullScreen(), 'isFullScreen').toBe(true);
  });

  it('can be added as a child of another View', async () => {
    const w = new BaseWindow();
    const v = new View();
    const wcv = new WebContentsView();

    await wcv.webContents.loadURL('data:text/html,<div id="div">This is a simple div.</div>');

    v.addChildView(wcv);
    w.contentView.addChildView(v);

    expect(w.contentView.children).toEqual([v]);
    expect(v.children).toEqual([wcv]);
  });

  describe('when moved to another window', () => {
    const moveAndAttachWebview = async (destroyFirstWindow: boolean) => {
      const wA = new BrowserWindow({ show: false });
      const wB = new BrowserWindow({ show: false });
      const view = new WebContentsView({ webPreferences: { webviewTag: true } });
      wA.contentView.addChildView(view);
      wB.contentView.addChildView(view);
      if (destroyFirstWindow) {
        const closed = once(wA, 'closed');
        wA.destroy();
        await closed;
      }
      const attached = once(view.webContents, 'did-attach-webview') as Promise<[any, Electron.WebContents]>;
      await view.webContents.loadURL('data:text/html,<webview src="about:blank"></webview>');
      const [, guest] = await attached;
      return { wB, guest };
    };

    it('gives a <webview> guest the new window as its owner', async () => {
      const { wB, guest } = await moveAndAttachWebview(false);
      expect(BrowserWindow.fromWebContents(guest)).toBe(wB);
    });

    it('gives a <webview> guest the new window as its owner after the old one is destroyed', async () => {
      const { wB, guest } = await moveAndAttachWebview(true);
      expect(BrowserWindow.fromWebContents(guest)).toBe(wB);
    });

    const attachWebviewAndMove = async (destroyFirstWindow: boolean) => {
      const wA = new BrowserWindow({ show: false });
      const wB = new BrowserWindow({ show: false });
      const view = new WebContentsView({ webPreferences: { webviewTag: true } });
      wA.contentView.addChildView(view);
      const attached = once(view.webContents, 'did-attach-webview') as Promise<[any, Electron.WebContents]>;
      await view.webContents.loadURL('data:text/html,<webview src="about:blank"></webview>');
      const [, guest] = await attached;
      expect(BrowserWindow.fromWebContents(guest)).toBe(wA);
      wB.contentView.addChildView(view);
      if (destroyFirstWindow) {
        const closed = once(wA, 'closed');
        wA.destroy();
        await closed;
      }
      return { wB, guest };
    };

    it('moves an attached <webview> guest to the new window', async () => {
      const { wB, guest } = await attachWebviewAndMove(false);
      expect(BrowserWindow.fromWebContents(guest)).toBe(wB);
    });

    it('moves an attached <webview> guest to the new window after the old one is destroyed', async () => {
      const { wB, guest } = await attachWebviewAndMove(true);
      expect(guest.isDestroyed()).toBe(false);
      expect(BrowserWindow.fromWebContents(guest)).toBe(wB);
    });

    for (const mode of ['bottom', 'detach'] as const) {
      it(`moves open DevTools (mode: ${mode}) to the new window`, async () => {
        const wA = new BrowserWindow({ show: false });
        const wB = new BrowserWindow({ show: false });
        const view = new WebContentsView();
        wA.contentView.addChildView(view);
        await view.webContents.loadURL('about:blank');
        const opened = once(view.webContents, 'devtools-opened');
        view.webContents.openDevTools({ mode, activate: false });
        await opened;
        const devtools = view.webContents.devToolsWebContents!;
        expect(BrowserWindow.fromWebContents(devtools)).toBe(wA);
        wB.contentView.addChildView(view);
        expect(BrowserWindow.fromWebContents(devtools)).toBe(wB);
      });
    }
  });

  describe('when removed from its window', () => {
    const attachWebview = async (w: BrowserWindow) => {
      const view = new WebContentsView({ webPreferences: { webviewTag: true } });
      w.contentView.addChildView(view);
      const attached = once(view.webContents, 'did-attach-webview') as Promise<[any, Electron.WebContents]>;
      await view.webContents.loadURL('data:text/html,<webview src="about:blank"></webview>');
      const [, guest] = await attached;
      expect(BrowserWindow.fromWebContents(view.webContents)).toBe(w);
      expect(BrowserWindow.fromWebContents(guest)).toBe(w);
      return { view, guest };
    };

    it('no longer has the window as its owner', async () => {
      const w = new BrowserWindow({ show: false });
      const { view, guest } = await attachWebview(w);
      w.contentView.removeChildView(view);
      expect(view.webContents.getOwnerBrowserWindow()).toBe(null);
      expect(BrowserWindow.fromWebContents(view.webContents)).toBe(null);
      expect(BrowserWindow.fromWebContents(guest)).toBe(null);
    });

    it('no longer has the window as its owner when its parent view is removed', async () => {
      const w = new BrowserWindow({ show: false });
      const parent = new View();
      w.contentView.addChildView(parent);
      const view = new WebContentsView();
      parent.addChildView(view);
      expect(BrowserWindow.fromWebContents(view.webContents)).toBe(w);
      w.contentView.removeChildView(parent);
      expect(BrowserWindow.fromWebContents(view.webContents)).toBe(null);
    });

    it('takes a window as its owner again when re-added', async () => {
      const wA = new BrowserWindow({ show: false });
      const wB = new BrowserWindow({ show: false });
      const { view, guest } = await attachWebview(wA);

      wA.contentView.removeChildView(view);
      wB.contentView.addChildView(view);
      expect(BrowserWindow.fromWebContents(view.webContents)).toBe(wB);
      expect(BrowserWindow.fromWebContents(guest)).toBe(wB);

      wB.contentView.removeChildView(view);
      wA.contentView.addChildView(view);
      expect(BrowserWindow.fromWebContents(view.webContents)).toBe(wA);
      expect(BrowserWindow.fromWebContents(guest)).toBe(wA);
    });

    it('keeps the size of an offscreen page', async () => {
      const w = new BaseWindow({ show: false, width: 100, height: 100 });
      const view = new WebContentsView({ webPreferences: { offscreen: true, backgroundThrottling: false } });
      w.contentView.addChildView(view);
      const paint = once(view.webContents, 'paint') as Promise<[any, Electron.Rectangle, Electron.NativeImage]>;
      await view.webContents.loadURL('data:text/html,<body style="background: red"></body>');
      const [, , attachedImage] = await paint;
      const attachedSize = attachedImage.getSize();
      expect(attachedSize.width).toBeGreaterThan(0);
      expect(attachedSize.height).toBeGreaterThan(0);
      const getPageSize = () => view.webContents.executeJavaScript('[innerWidth, innerHeight]');
      const attachedPageSize = await getPageSize();

      w.contentView.removeChildView(view);
      const repaint = once(view.webContents, 'paint') as Promise<[any, Electron.Rectangle, Electron.NativeImage]>;
      view.webContents.invalidate();
      const [, , detachedImage] = await repaint;
      expect(detachedImage.getSize()).toEqual(attachedSize);
      expect(await getPageSize()).toEqual(attachedPageSize);
    });

    it('does not fullscreen the old window', { tags: ['serial'] }, async () => {
      const w = new BaseWindow();
      const view = new WebContentsView();
      w.contentView.addChildView(view);
      await view.webContents.loadURL('data:text/html,<div id="div">This is a simple div.</div>');
      w.contentView.removeChildView(view);

      let entered = false;
      view.webContents.once('enter-html-full-screen', () => {
        entered = true;
      });
      w.once('enter-full-screen', () => {
        entered = true;
      });
      // The request is not granted, so there is nothing to wait for; give it
      // ample time to reach the window if it were going to.
      view.webContents.executeJavaScript('document.getElementById("div").requestFullscreen()', true).catch(() => {});
      await setTimeoutAsync(1000);
      expect(entered).toBe(false);
      expect(w.isFullScreen()).toBe(false);
    });

    const enterHtmlFullScreen = async (w: BaseWindow, view: WebContentsView) => {
      await view.webContents.loadURL('data:text/html,<div id="div">This is a simple div.</div>');
      const enteredHtmlFullScreen = once(view.webContents, 'enter-html-full-screen');
      const enteredFullScreen = once(w, 'enter-full-screen');
      await view.webContents.executeJavaScript('document.getElementById("div").requestFullscreen()', true);
      await enteredHtmlFullScreen;
      await enteredFullScreen;
    };

    it('leaves HTML fullscreen', { tags: ['serial'] }, async () => {
      const w = new BaseWindow();
      const view = new WebContentsView();
      w.contentView.addChildView(view);
      await enterHtmlFullScreen(w, view);

      const leaveHtmlFullScreen = once(view.webContents, 'leave-html-full-screen');
      const leaveFullScreen = once(w, 'leave-full-screen');
      w.contentView.removeChildView(view);
      await leaveHtmlFullScreen;
      await leaveFullScreen;
      expect(w.isFullScreen()).toBe(false);
      await waitUntil(() => view.webContents.executeJavaScript('document.fullscreenElement === null'));
      expect(BrowserWindow.fromWebContents(view.webContents)).toBe(null);
    });

    // Leaving HTML fullscreen emits events to JavaScript, which must not run
    // while the view is still being removed.
    it(
      'does not crash when leaving HTML fullscreen adds a view to the removed parent',
      { tags: ['serial'] },
      async () => {
        const w = new BaseWindow();
        const parent = new View();
        w.contentView.addChildView(parent);
        const view = new WebContentsView();
        parent.addChildView(view);
        await enterHtmlFullScreen(w, view);

        const leaveHtmlFullScreen = once(view.webContents, 'leave-html-full-screen');
        const leaveFullScreen = once(w, 'leave-full-screen');
        view.webContents.once('leave-html-full-screen', () => {
          parent.addChildView(new View());
        });
        w.contentView.removeChildView(parent);
        await leaveHtmlFullScreen;
        await leaveFullScreen;
        expect(parent.children.length).toBe(2);
        await waitUntil(() => view.webContents.executeJavaScript('document.fullscreenElement === null'));
        expect(BrowserWindow.fromWebContents(view.webContents)).toBe(null);
      }
    );

    it('does not crash when leaving HTML fullscreen removes a sibling', { tags: ['serial'] }, async () => {
      const w = new BaseWindow();
      const sibling = new View();
      w.contentView.addChildView(sibling);
      const view = new WebContentsView();
      w.contentView.addChildView(view);
      await enterHtmlFullScreen(w, view);

      const leaveHtmlFullScreen = once(w, 'leave-html-full-screen');
      const leaveFullScreen = once(w, 'leave-full-screen');
      // BaseWindow emits this at runtime, but it is only typed on BrowserWindow.
      (w as NodeJS.EventEmitter).once('leave-html-full-screen', () => {
        w.contentView.removeChildView(sibling);
      });
      w.contentView.removeChildView(view);
      await leaveHtmlFullScreen;
      await leaveFullScreen;
      expect(w.contentView.children.length).toBe(0);
      await waitUntil(() => view.webContents.executeJavaScript('document.fullscreenElement === null'));
      expect(BrowserWindow.fromWebContents(view.webContents)).toBe(null);
    });

    it(
      'does not crash when leaving HTML fullscreen adds the view to another window',
      { tags: ['serial'] },
      async () => {
        const wA = new BaseWindow();
        const wB = new BaseWindow({ show: false });
        const view = new WebContentsView();
        wA.contentView.addChildView(view);
        await enterHtmlFullScreen(wA, view);

        const leaveHtmlFullScreen = once(view.webContents, 'leave-html-full-screen');
        const leaveFullScreen = once(wA, 'leave-full-screen');
        view.webContents.once('leave-html-full-screen', () => {
          wB.contentView.addChildView(view);
        });
        wA.contentView.removeChildView(view);
        await leaveHtmlFullScreen;
        await leaveFullScreen;
        await waitUntil(() => view.webContents.executeJavaScript('document.fullscreenElement === null'));
        expect(wB.contentView.children[0]).toBe(view);
        expect(BrowserWindow.fromWebContents(view.webContents)).toBe(wB);
      }
    );

    it(
      'keeps the window as its owner when leaving HTML fullscreen adds the view to it again',
      { tags: ['serial'] },
      async () => {
        const w = new BaseWindow();
        const view = new WebContentsView();
        w.contentView.addChildView(view);
        await enterHtmlFullScreen(w, view);

        const leaveHtmlFullScreen = once(view.webContents, 'leave-html-full-screen');
        const leaveFullScreen = once(w, 'leave-full-screen');
        view.webContents.once('leave-html-full-screen', () => {
          w.contentView.addChildView(view);
        });
        w.contentView.removeChildView(view);
        await leaveHtmlFullScreen;
        await leaveFullScreen;
        await waitUntil(() => view.webContents.executeJavaScript('document.fullscreenElement === null'));
        expect(w.contentView.children[0]).toBe(view);
        expect(BrowserWindow.fromWebContents(view.webContents)).toBe(w);
      }
    );

    // Another page's HTML fullscreen is not the removed page's to leave.
    it('does not leave HTML fullscreen when another view is removed', { tags: ['serial'] }, async () => {
      const w = new BaseWindow();
      const other = new WebContentsView();
      w.contentView.addChildView(other);
      await other.webContents.loadURL('about:blank');
      const view = new WebContentsView();
      w.contentView.addChildView(view);
      await enterHtmlFullScreen(w, view);

      let left = false;
      view.webContents.once('leave-html-full-screen', () => {
        left = true;
      });
      w.once('leave-full-screen', () => {
        left = true;
      });
      w.contentView.removeChildView(other);
      expect(BrowserWindow.fromWebContents(other.webContents)).toBe(null);
      // Nothing is expected to happen; give it time to reach the page if it were.
      await setTimeoutAsync(500);
      expect(left).toBe(false);
      expect(w.isFullScreen()).toBe(true);
      expect(await view.webContents.executeJavaScript('document.fullscreenElement !== null')).toBe(true);
      expect(BrowserWindow.fromWebContents(view.webContents)).toBe(w);
    });

    it('does not crash when the window closes while the page is in HTML fullscreen', { tags: ['serial'] }, async () => {
      const w = new BaseWindow();
      const view = new WebContentsView();
      w.contentView.addChildView(view);
      await enterHtmlFullScreen(w, view);

      const closed = once(w, 'closed');
      w.close();
      await closed;
      // The native window is destroyed in a task after 'closed'.
      await setTimeoutAsync(100);

      expect(view.webContents.isDestroyed()).toBe(false);
      expect(BrowserWindow.fromWebContents(view.webContents)).toBe(null);
      expect(await view.webContents.executeJavaScript('1 + 1')).toBe(2);
    });

    for (const mode of ['bottom', 'detach'] as const) {
      it(`does not crash when the window closes with a <webview> guest and DevTools (mode: ${mode})`, async () => {
        const w = new BrowserWindow({ show: false });
        const { view, guest } = await attachWebview(w);
        const opened = once(view.webContents, 'devtools-opened');
        view.webContents.openDevTools({ mode, activate: false });
        await opened;

        const closed = once(w, 'closed');
        w.close();
        await closed;
        // The native window is destroyed in a task after 'closed'.
        await setTimeoutAsync(100);

        expect(view.webContents.isDestroyed()).toBe(false);
        expect(guest.isDestroyed()).toBe(false);
        expect(BrowserWindow.fromWebContents(view.webContents)).toBe(null);
        expect(BrowserWindow.fromWebContents(guest)).toBe(null);
        expect(await view.webContents.executeJavaScript('1 + 1')).toBe(2);
      });
    }
  });

  describe('visibilityState', { tags: ['serial'] }, () => {
    // These pages are only 'visible' while nothing covers their window. On the
    // Windows CI hosts another process's console window can sit above a newly
    // shown window, and Chromium's native occlusion tracker then keeps the page
    // 'hidden'. A BrowserWindow's own webContents is force-shown, but BaseWindow
    // content is not, so keep these windows above everything there (as
    // visibility-state.spec.ts does).
    const alwaysOnTop = process.platform === 'win32';

    async function haveVisibilityState(view: WebContentsView, state: string) {
      const docVisState = await view.webContents.executeJavaScript('document.visibilityState');
      return docVisState === state;
    }

    it('is initially hidden', async () => {
      const v = new WebContentsView();
      await v.webContents.loadURL('data:text/html,<script>initialVisibility = document.visibilityState</script>');
      expect(await v.webContents.executeJavaScript('initialVisibility')).toBe('hidden');
    });

    it('becomes visible when attached', async () => {
      const v = new WebContentsView();
      await v.webContents.loadURL('about:blank');
      expect(await v.webContents.executeJavaScript('document.visibilityState')).toBe('hidden');
      const p = v.webContents.executeJavaScript(
        'new Promise(resolve => document.addEventListener("visibilitychange", resolve))'
      );
      // Ensure that the above listener has been registered before we add the
      // view to the window, or else the visibilitychange event might be
      // dispatched before the listener is registered.
      // executeJavaScript calls are sequential so if this one's finished then
      // the previous one must also have been finished :)
      await v.webContents.executeJavaScript('undefined');
      const w = new BaseWindow({ alwaysOnTop, width: 400, height: 300 });
      w.setContentView(v);
      await p;
      expect(await v.webContents.executeJavaScript('document.visibilityState')).toBe('visible');

      // These two new `expect` calls added in 2026-04-20 @ 2e17a57 are causing
      // CI flakes. https://github.com/electron/electron/issues/51228
      // TODO(ckerr) fix the flakes and re-enable
      //
      // const viewportSize = await v.webContents.executeJavaScript(
      //   '({ width: window.innerWidth, height: window.innerHeight })'
      // );
      // const contentBounds = w.getContentBounds();
      // expect(viewportSize.width).to.equal(contentBounds.width);
      // expect(viewportSize.height).to.equal(contentBounds.height);
    });

    it('is initially visible if load happens after attach', async () => {
      const w = new BaseWindow({ alwaysOnTop });
      const v = new WebContentsView();
      w.contentView = v;
      await v.webContents.loadURL('data:text/html,<script>initialVisibility = document.visibilityState</script>');
      expect(await v.webContents.executeJavaScript('initialVisibility')).toBe('visible');
    });

    it('becomes hidden when parent window is hidden', async () => {
      const w = new BaseWindow({ alwaysOnTop });
      const v = new WebContentsView();
      w.setContentView(v);
      await v.webContents.loadURL('about:blank');
      await waitUntil(async () => await haveVisibilityState(v, 'visible'));
      const p = v.webContents.executeJavaScript(
        'new Promise(resolve => document.addEventListener("visibilitychange", resolve))'
      );
      // We have to wait until the listener above is fully registered before hiding the window.
      // On Windows, the executeJavaScript and the visibilitychange can happen out of order
      // without this.
      await v.webContents.executeJavaScript('0');
      w.hide();
      await p;
      expect(await v.webContents.executeJavaScript('document.visibilityState')).toBe('hidden');
    });

    it('becomes visible when parent window is shown', async () => {
      const w = new BaseWindow({ alwaysOnTop, show: false });
      const v = new WebContentsView();
      w.setContentView(v);
      await v.webContents.loadURL('about:blank');
      expect(await v.webContents.executeJavaScript('document.visibilityState')).toBe('hidden');
      const p = v.webContents.executeJavaScript(
        'new Promise(resolve => document.addEventListener("visibilitychange", resolve))'
      );
      // We have to wait until the listener above is fully registered before hiding the window.
      // On Windows, the executeJavaScript and the visibilitychange can happen out of order
      // without this.
      await v.webContents.executeJavaScript('0');
      w.show();
      await p;
      expect(await v.webContents.executeJavaScript('document.visibilityState')).toBe('visible');
    });

    it('does not change when view is moved between two visible windows', async () => {
      const w = new BaseWindow({ alwaysOnTop });
      const v = new WebContentsView();
      w.setContentView(v);
      await v.webContents.loadURL('about:blank');
      await waitUntil(async () => await haveVisibilityState(v, 'visible'));

      const p = v.webContents.executeJavaScript(
        'new Promise(resolve => document.addEventListener("visibilitychange", () => resolve(document.visibilityState)))'
      );
      // Ensure the listener has been registered.
      await v.webContents.executeJavaScript('undefined');
      const w2 = new BaseWindow({ alwaysOnTop });
      w2.setContentView(v);
      // Wait for the visibility state to settle as "visible".
      // On macOS one visibilitychange event is fired but visibilityState
      // remains "visible". On Win/Linux, two visibilitychange events are
      // fired, a "hidden" and a "visible" one. Reconcile these two models
      // by waiting until at least one event has been fired, and then waiting
      // until the visibility state settles as "visible".
      let visibilityState = await p;
      for (let attempts = 0; visibilityState !== 'visible' && attempts < 10; attempts++) {
        visibilityState = await v.webContents.executeJavaScript(
          'new Promise(resolve => document.visibilityState === "visible" ? resolve("visible") : document.addEventListener("visibilitychange", () => resolve(document.visibilityState)))'
        );
      }
      expect(visibilityState).toBe('visible');
    });

    it('tracks visibility for multiple child WebContentsViews', async () => {
      const w = new BaseWindow({ alwaysOnTop, show: false });
      const cv = new View();
      w.setContentView(cv);

      const v1 = new WebContentsView();
      const v2 = new WebContentsView();
      cv.addChildView(v1);
      cv.addChildView(v2);
      v1.setBounds({ x: 0, y: 0, width: 400, height: 300 });
      v2.setBounds({ x: 0, y: 300, width: 400, height: 300 });

      await v1.webContents.loadURL('about:blank');
      await v2.webContents.loadURL('about:blank');

      await waitUntil(async () => await haveVisibilityState(v1, 'hidden'));
      await waitUntil(async () => await haveVisibilityState(v2, 'hidden'));

      w.show();

      await waitUntil(async () => await haveVisibilityState(v1, 'visible'));
      await waitUntil(async () => await haveVisibilityState(v2, 'visible'));

      w.hide();

      await waitUntil(async () => await haveVisibilityState(v1, 'hidden'));
      await waitUntil(async () => await haveVisibilityState(v2, 'hidden'));
    });

    it('tracks visibility independently when a child WebContentsView is hidden via setVisible', async () => {
      const w = new BaseWindow({ alwaysOnTop });
      const cv = new View();
      w.setContentView(cv);

      const v1 = new WebContentsView();
      const v2 = new WebContentsView();
      cv.addChildView(v1);
      cv.addChildView(v2);
      v1.setBounds({ x: 0, y: 0, width: 400, height: 300 });
      v2.setBounds({ x: 0, y: 300, width: 400, height: 300 });

      await v1.webContents.loadURL('about:blank');
      await v2.webContents.loadURL('about:blank');

      await waitUntil(async () => await haveVisibilityState(v1, 'visible'));
      await waitUntil(async () => await haveVisibilityState(v2, 'visible'));

      v1.setVisible(false);

      await waitUntil(async () => await haveVisibilityState(v1, 'hidden'));
      // v2 should remain visible while v1 is hidden
      expect(await v2.webContents.executeJavaScript('document.visibilityState')).toBe('visible');

      v1.setVisible(true);

      await waitUntil(async () => await haveVisibilityState(v1, 'visible'));
    });

    it('fires a single visibilitychange event per show/hide transition', async () => {
      const w = new BaseWindow({ alwaysOnTop, show: false });
      const v = new WebContentsView();
      w.setContentView(v);
      await v.webContents.loadURL('about:blank');

      await v.webContents.executeJavaScript(`
        window.__visChanges = [];
        document.addEventListener('visibilitychange', () => {
          window.__visChanges.push(document.visibilityState);
        });
      `);

      w.show();
      await waitUntil(async () => await haveVisibilityState(v, 'visible'));

      // Give any delayed/queued occlusion updates time to fire.
      await setTimeoutAsync(1500);

      w.hide();
      await waitUntil(async () => await haveVisibilityState(v, 'hidden'));

      await setTimeoutAsync(1500);

      const changes = await v.webContents.executeJavaScript('window.__visChanges');
      // Expect exactly one 'visible' followed by one 'hidden'. Extra events
      // would indicate the occlusion checker is causing spurious transitions.
      expect(changes).toEqual(['visible', 'hidden']);
    });
  });

  describe('setBounds', () => {
    it('sizes the page when the view is not attached to a window', async () => {
      const v = new WebContentsView();
      v.setBounds({ x: 0, y: 0, width: 400, height: 300 });
      await v.webContents.loadURL('data:text/html,<script>initialSize = [innerWidth, innerHeight]</script>');
      expect(await v.webContents.executeJavaScript('initialSize')).toEqual([400, 300]);
      expect(await v.webContents.executeJavaScript('[innerWidth, innerHeight]')).toEqual([400, 300]);
    });

    it('resizes the page when bounds are set after loading', async () => {
      const v = new WebContentsView();
      v.setBounds({ x: 0, y: 0, width: 400, height: 300 });
      await v.webContents.loadURL('about:blank');
      v.setBounds({ x: 0, y: 0, width: 500, height: 400 });
      await waitUntil(async () => {
        const size = await v.webContents.executeJavaScript('[innerWidth, innerHeight]');
        return size[0] === 500 && size[1] === 400;
      });
    });

    it('resizes the page after the view is removed from a window', async () => {
      const w = new BaseWindow({ width: 400, height: 300 });
      const v = new WebContentsView();
      w.contentView.addChildView(v);
      v.setBounds({ x: 0, y: 0, width: 400, height: 300 });
      await v.webContents.loadURL('about:blank');
      w.contentView.removeChildView(v);
      v.setBounds({ x: 0, y: 0, width: 500, height: 400 });
      await waitUntil(async () => {
        const size = await v.webContents.executeJavaScript('[innerWidth, innerHeight]');
        return size[0] === 500 && size[1] === 400;
      });
    });
  });

  describe('setBorderRadius', () => {
    ifdescribe(hasCapturableScreen())('capture', { tags: ['serial'] }, () => {
      let w: Electron.BaseWindow;
      let v: Electron.WebContentsView;

      const backgroundUrl = `data:text/html,<style>html{background:${encodeURIComponent(HexColors.GREEN)}}</style>`;

      // Points just inside each corner of the captured window, which lie
      // within the cutout while a border radius is applied.
      const inset = 10;
      const corners: Array<(size: Electron.Size) => Electron.Point> = [
        () => ({ x: inset, y: inset }), // top-left
        ({ width }) => ({ x: width - inset, y: inset }), // top-right
        ({ width, height }) => ({ x: width - inset, y: height - inset }), // bottom-right
        ({ height }) => ({ x: inset, y: height - inset }) // bottom-left
      ];

      beforeEach(async () => {
        w = new BaseWindow({
          ...screen.getPrimaryDisplay().workArea,
          show: true,
          frame: false,
          hasShadow: false,
          backgroundColor: HexColors.BLUE,
          roundedCorners: false
        });

        v = new WebContentsView();
        w.setContentView(v);
        v.setBorderRadius(100);

        const readyForCapture = once(v.webContents, 'ready-to-show');
        v.webContents.loadURL(backgroundUrl);
        await readyForCapture;
      });

      afterEach(() => {
        w.destroy();
        w = v = null!;
      });

      it('should render with cutout corners', async () => {
        const capture = ScreenCapture.forWindow(w);

        for (const corner of corners) {
          await capture.expectColorAtPointMatches(HexColors.BLUE, corner);
        }

        // Center should be WebContents page background color
        await capture.expectColorAtCenterMatches(HexColors.GREEN);
      });

      it('should allow resetting corners', async () => {
        v.setBorderRadius(0);

        await nextFrameTime();
        const capture = ScreenCapture.forWindow(w);
        await capture.expectColorAtPointMatches(HexColors.GREEN, corners[0]);
        await capture.expectColorAtCenterMatches(HexColors.GREEN);
      });

      it('should render when set before attached', async () => {
        v = new WebContentsView();
        v.setBorderRadius(100); // must set before

        w.setContentView(v);

        const readyForCapture = once(v.webContents, 'ready-to-show');
        v.webContents.loadURL(backgroundUrl);
        await readyForCapture;

        const capture = ScreenCapture.forWindow(w);
        await capture.expectColorAtPointMatches(HexColors.BLUE, corners[0]);
        await capture.expectColorAtCenterMatches(HexColors.GREEN);
      });
    });

    it('should allow setting when not attached', async () => {
      const v = new WebContentsView();
      v.setBorderRadius(100);
    });
  });

  describe('focusOnNavigation webPreference', { tags: ['serial'] }, () => {
    it('focuses the webContents on navigation by default', async () => {
      const w = new BrowserWindow();
      await once(w, 'focus');
      const v = new WebContentsView();
      w.setContentView(v);
      await v.webContents.loadURL('about:blank');
      const devToolsFocused = once(v.webContents, 'devtools-focused');
      v.webContents.openDevTools({ mode: 'right' });
      await devToolsFocused;
      expect(v.webContents.isFocused()).toBe(false);
      const focused = once(v.webContents, 'focus');
      await v.webContents.loadURL('data:text/html,<body>test</body>');
      await focused;
      expect(v.webContents.isFocused()).toBe(true);
    });

    it('does not focus the webContents on navigation when focusOnNavigation is false', async () => {
      const w = new BrowserWindow();
      await once(w, 'focus');
      const v = new WebContentsView({
        webPreferences: {
          focusOnNavigation: false
        }
      });
      w.setContentView(v);
      await v.webContents.loadURL('about:blank');
      const devToolsFocused = once(v.webContents, 'devtools-focused');
      v.webContents.openDevTools({ mode: 'right' });
      await devToolsFocused;
      expect(v.webContents.isFocused()).toBe(false);
      await v.webContents.loadURL('data:text/html,<body>test</body>');
      expect(v.webContents.isFocused()).toBe(false);
    });
  });
});
