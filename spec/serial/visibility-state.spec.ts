import {
  BaseWindow,
  BrowserWindow,
  BrowserWindowConstructorOptions,
  webContents,
  WebContents,
  WebContentsView
} from 'electron/main';

import { expect } from 'chai';
import { afterEach, beforeAll, it, type TestFunction, type TestOptions } from 'vitest';

import * as cp from 'node:child_process';
import { once } from 'node:events';
import * as path from 'node:path';

import { ifdescribe, waitUntil } from '../lib/spec-helpers';
import { closeAllWindows } from '../lib/window-helpers';

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
    w.webContents.loadFile(path.resolve(__dirname, '..', 'fixtures', 'chromium', 'visibilitystate.html'));

  async function haveVisibilityState(state: string) {
    const docVisState = await w.webContents.executeJavaScript('document.visibilityState');
    return docVisState === state;
  }

  const itWithOptions = (
    name: string,
    options: BrowserWindowConstructorOptions,
    fn: TestFunction,
    testOptions: TestOptions = {}
  ) => {
    it(name, testOptions, async (ctx) => {
      w = new BrowserWindow({
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
      await fn(ctx);
    });

    it(name + ' with BaseWindow', testOptions, async (ctx) => {
      const baseWindow = new BaseWindow({
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
      await fn(ctx);
    });
  };

  itWithOptions('should be visible when the window is initially shown by default', {}, async (ctx) => {
    load();
    await expect(waitUntil(async () => await haveVisibilityState('visible'), ctx.signal)).to.eventually.be.fulfilled();
  });

  itWithOptions(
    'should be visible when the window is initially shown',
    {
      show: true
    },
    async (ctx) => {
      load();
      await expect(
        waitUntil(async () => await haveVisibilityState('visible'), ctx.signal)
      ).to.eventually.be.fulfilled();
    }
  );

  itWithOptions(
    'should be hidden when the window is initially hidden',
    {
      show: false
    },
    async (ctx) => {
      load();
      await expect(waitUntil(async () => await haveVisibilityState('hidden'), ctx.signal)).to.eventually.be.fulfilled();
    }
  );

  itWithOptions(
    'should be visible when the window is initially hidden but shown before the page is loaded',
    {
      show: false
    },
    async (ctx) => {
      w.show();
      load();
      await expect(
        waitUntil(async () => await haveVisibilityState('visible'), ctx.signal)
      ).to.eventually.be.fulfilled();
    }
  );

  itWithOptions(
    'should be hidden when the window is initially shown but hidden before the page is loaded',
    {
      show: true
    },
    async (ctx) => {
      w.hide();
      load();
      await expect(waitUntil(async () => await haveVisibilityState('hidden'), ctx.signal)).to.eventually.be.fulfilled();
    }
  );

  itWithOptions('should be toggle between visible and hidden as the window is hidden and shown', {}, async (ctx) => {
    load();
    await expect(waitUntil(async () => await haveVisibilityState('visible'), ctx.signal)).to.eventually.be.fulfilled();
    w.hide();
    await expect(waitUntil(async () => await haveVisibilityState('hidden'), ctx.signal)).to.eventually.be.fulfilled();
    w.show();
    await expect(waitUntil(async () => await haveVisibilityState('visible'), ctx.signal)).to.eventually.be.fulfilled();
  });

  itWithOptions('should become hidden when a window is minimized', {}, async (ctx) => {
    load();
    await expect(waitUntil(async () => await haveVisibilityState('visible'), ctx.signal)).to.eventually.be.fulfilled();
    w.minimize();
    await expect(waitUntil(async () => await haveVisibilityState('hidden'), ctx.signal)).to.eventually.be.fulfilled();
  });

  itWithOptions('should become visible when a window is restored', {}, async (ctx) => {
    load();
    await expect(waitUntil(async () => await haveVisibilityState('visible'), ctx.signal)).to.eventually.be.fulfilled();
    w.minimize();
    await expect(waitUntil(async () => await haveVisibilityState('hidden'), ctx.signal)).to.eventually.be.fulfilled();
    w.restore();
    await expect(waitUntil(async () => await haveVisibilityState('visible'), ctx.signal)).to.eventually.be.fulfilled();
  });

  ifdescribe(process.platform === 'darwin')('on platforms that support occlusion detection', () => {
    let child: cp.ChildProcess;

    const makeOtherWindow = (opts: { x: number; y: number; width: number; height: number }) => {
      child = cp.spawn(process.execPath, [
        path.resolve(__dirname, '..', 'fixtures', 'chromium', 'other-window.js'),
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
      async (ctx) => {
        await makeOtherWindow({
          x: 200,
          y: 0,
          width: 200,
          height: 200
        });
        load();
        await expect(
          waitUntil(async () => await haveVisibilityState('visible'), ctx.signal)
        ).to.eventually.be.fulfilled();
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
      async (ctx) => {
        await makeOtherWindow({
          x: 100,
          y: 0,
          width: 200,
          height: 200
        });
        load();
        await expect(
          waitUntil(async () => await haveVisibilityState('visible'), ctx.signal)
        ).to.eventually.be.fulfilled();
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
      async (ctx) => {
        load();
        await expect(
          waitUntil(async () => await haveVisibilityState('visible'), ctx.signal)
        ).to.eventually.be.fulfilled();
        makeOtherWindow({
          x: 0,
          y: 0,
          width: 300,
          height: 300
        });
        await expect(
          waitUntil(async () => await haveVisibilityState('hidden'), ctx.signal)
        ).to.eventually.be.fulfilled();
      },
      { timeout: 240000 }
    );
  });
});
