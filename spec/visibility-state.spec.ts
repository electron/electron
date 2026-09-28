import {
  BaseWindow,
  BrowserWindow,
  type BrowserWindowConstructorOptions,
  webContents,
  type WebContents,
  WebContentsView
} from 'electron/main';

import { afterEach, beforeAll, expect, it } from 'vitest';

import * as cp from 'node:child_process';
import { once } from 'node:events';
import * as path from 'node:path';
import { setTimeout } from 'node:timers/promises';

import { ifdescribe, waitUntil } from './lib/spec-helpers.ts';
import { closeAllWindows } from './lib/window-helpers.ts';

// visibilityState specs pass on linux with a real window manager but on CI
// the environment does not let these specs pass
ifdescribe(process.platform !== 'linux')('document.visibilityState', () => {
  let w: BaseWindow & { webContents: WebContents };

  beforeAll(() => {
    for (const checkWin of BaseWindow.getAllWindows()) {
      console.log('WINDOW EXISTS BEFORE TEST STARTED:', checkWin.title, checkWin.id);
    }
  });

  afterEach(async () => {
    await closeAllWindows();
    w = null as unknown as BrowserWindow;
    const existingWCS = webContents.getAllWebContents();
    existingWCS.forEach((contents) => contents.close());
  });

  const load = () =>
    w.webContents.loadFile(path.resolve(import.meta.dirname, 'fixtures', 'chromium', 'visibilitystate.html'));

  async function haveVisibilityState(state: string) {
    const docVisState = await w.webContents.executeJavaScript('document.visibilityState');
    return docVisState === state;
  }

  // On the Windows CI hosts another process's console window can end up
  // above a newly shown test window, and Chromium's native occlusion tracker
  // then reports the page as 'hidden'. Keep the window above everything there
  // so the desktop's z-order cannot decide the outcome. macOS runs the
  // occlusion specs below, which need a normal window level.
  const alwaysOnTop = process.platform === 'win32';

  const itWithOptions = (
    name: string,
    options: BrowserWindowConstructorOptions,
    fn: () => unknown,
    testOptions: { timeout?: number } = {}
  ) => {
    it(name, testOptions, async () => {
      w = new BrowserWindow({
        alwaysOnTop,
        ...options,
        paintWhenInitiallyHidden: false,
        webPreferences: {
          ...(options.webPreferences || {}),
          nodeIntegration: true,
          contextIsolation: false
        }
      });
      if (options.show && process.platform === 'darwin') {
        await once(w, 'show');
      }
      await fn();
    });

    it(name + ' with BaseWindow', testOptions, async () => {
      const baseWindow = new BaseWindow({
        alwaysOnTop,
        ...options
      });
      const wcv = new WebContentsView({
        webPreferences: { ...(options.webPreferences ?? {}), nodeIntegration: true, contextIsolation: false }
      });
      baseWindow.contentView = wcv;
      w = Object.assign(baseWindow, { webContents: wcv.webContents });
      if (options.show && process.platform === 'darwin') {
        await once(w, 'show');
      }
      await fn();
    });
  };

  itWithOptions('should be visible when the window is initially shown by default', {}, async () => {
    load();
    await waitUntil(async () => await haveVisibilityState('visible'));
  });

  itWithOptions(
    'should be visible when the window is initially shown',
    {
      show: true
    },
    async () => {
      load();
      await waitUntil(async () => await haveVisibilityState('visible'));
    }
  );

  itWithOptions(
    'should be hidden when the window is initially hidden',
    {
      show: false
    },
    async () => {
      load();
      await waitUntil(async () => await haveVisibilityState('hidden'));
    }
  );

  itWithOptions(
    'should be visible when the window is initially hidden but shown before the page is loaded',
    {
      show: false
    },
    async () => {
      w.show();
      load();
      await waitUntil(async () => await haveVisibilityState('visible'));
    }
  );

  itWithOptions(
    'should be hidden when the window is initially shown but hidden before the page is loaded',
    {
      show: true
    },
    async () => {
      w.hide();
      load();
      await waitUntil(async () => await haveVisibilityState('hidden'));
    }
  );

  itWithOptions('should be toggle between visible and hidden as the window is hidden and shown', {}, async () => {
    load();
    await waitUntil(async () => await haveVisibilityState('visible'));
    w.hide();
    await waitUntil(async () => await haveVisibilityState('hidden'));
    w.show();
    await waitUntil(async () => await haveVisibilityState('visible'));
  });

  itWithOptions('should become hidden when a window is minimized', {}, async () => {
    load();
    await waitUntil(async () => await haveVisibilityState('visible'));
    w.minimize();
    await waitUntil(async () => await haveVisibilityState('hidden'));
  });

  itWithOptions('should become visible when a window is restored', {}, async () => {
    load();
    await waitUntil(async () => await haveVisibilityState('visible'));
    w.minimize();
    await waitUntil(async () => await haveVisibilityState('hidden'));
    w.restore();
    await waitUntil(async () => await haveVisibilityState('visible'));
  });

  ifdescribe(process.platform === 'darwin')('on platforms that support occlusion detection', () => {
    let child: cp.ChildProcess;

    const makeOtherWindow = (opts: { x: number; y: number; width: number; height: number }) => {
      child = cp.spawn(process.execPath, [
        path.resolve(import.meta.dirname, 'fixtures', 'chromium', 'other-window.js'),
        `${opts.x}`,
        `${opts.y}`,
        `${opts.width}`,
        `${opts.height}`
      ]);
      return new Promise<void>((resolve) => {
        child.stdout!.on('data', (chunk) => {
          if (chunk.toString().includes('__ready__')) resolve();
        });
      });
    };

    afterEach(() => {
      if (child && !child.killed) {
        child.kill('SIGTERM');
      }
    });

    itWithOptions(
      'should be visible when two windows are on screen',
      {
        x: 0,
        y: 0,
        width: 200,
        height: 200
      },
      async () => {
        await makeOtherWindow({
          x: 200,
          y: 0,
          width: 200,
          height: 200
        });
        load();
        await waitUntil(async () => await haveVisibilityState('visible'));
      }
    );

    itWithOptions(
      'should be visible when two windows are on screen that overlap partially',
      {
        x: 50,
        y: 50,
        width: 150,
        height: 150
      },
      async () => {
        await makeOtherWindow({
          x: 100,
          y: 0,
          width: 200,
          height: 200
        });
        load();
        await waitUntil(async () => await haveVisibilityState('visible'));
      }
    );

    itWithOptions(
      'should be hidden when a second window completely occludes the current window',
      {
        x: 50,
        y: 50,
        width: 50,
        height: 50
      },
      async () => {
        load();
        await waitUntil(async () => await haveVisibilityState('visible'));
        makeOtherWindow({
          x: 0,
          y: 0,
          width: 300,
          height: 300
        });
        await waitUntil(async () => await haveVisibilityState('hidden'));
      },
      { timeout: 240000 }
    );

    // https://github.com/electron/electron/issues/51718
    itWithOptions(
      'should stay visible when covered by a transparent click-through overlay',
      {
        x: 100,
        y: 100,
        width: 200,
        height: 200
      },
      async () => {
        load();
        await waitUntil(async () => await haveVisibilityState('visible'));

        const overlay = new BrowserWindow({
          x: 50,
          y: 50,
          width: 400,
          height: 400,
          show: false,
          transparent: true,
          frame: false,
          alwaysOnTop: true,
          focusable: false
        });
        overlay.setIgnoreMouseEvents(true);
        await overlay.loadURL('about:blank');
        overlay.showInactive();

        // Give the occlusion checker time to run; the covered window must not
        // be marked occluded by a window that can't obscure it.
        await setTimeout(2000);
        expect(await w.webContents.executeJavaScript('document.visibilityState')).to.equal('visible');
      }
    );
  });
});
