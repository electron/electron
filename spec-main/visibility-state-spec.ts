import { expect } from 'chai';
import * as cp from 'child_process';
import { BrowserWindow, BrowserWindowConstructorOptions, ipcMain } from 'electron/main';
import * as path from 'path';

import { emittedOnce } from './events-helpers';
import { closeWindow } from './window-helpers';
import { ifdescribe, delay } from './spec-helpers';

// visibilityState specs pass on linux with a real window manager but on CI
// the environment does not let these specs pass
ifdescribe(process.platform !== 'linux')('document.visibilityState', () => {
  let w: BrowserWindow;

  afterEach(() => {
    return closeWindow(w);
  });

  const load = () => w.loadFile(path.resolve(__dirname, 'fixtures', 'chromium', 'visibilitystate.html'));

  const itWithOptions = (name: string, options: BrowserWindowConstructorOptions, fn: Mocha.Func) => {
    return it(name, async function (...args) {
      // document.visibilityState tests are very flaky, this is probably because
      // Electron implements it via async IPC messages.
      this.retries(3);

      w = new BrowserWindow({
        ...options,
        paintWhenInitiallyHidden: false,
        webPreferences: {
          ...(options.webPreferences || {}),
          nodeIntegration: true
        }
      });
      await Promise.resolve(fn.apply(this, args));
    });
  };

  const getVisibilityState = async (): Promise<string> => {
    const response = emittedOnce(ipcMain, 'visibility-state');
    w.webContents.send('get-visibility-state');
    return (await response)[1];
  };

  itWithOptions('should be visible when the window is initially shown by default', {}, async () => {
    await load();
    const state = await getVisibilityState();
    expect(state).to.equal('visible');
  });

  itWithOptions('should be visible when the window is initially shown', {
    show: true
  }, async () => {
    await load();
    const state = await getVisibilityState();
    expect(state).to.equal('visible');
  });

  itWithOptions('should be hidden when the window is initially hidden', {
    show: false
  }, async () => {
    await load();
    const state = await getVisibilityState();
    expect(state).to.equal('hidden');
  });

  itWithOptions('should be visible when the window is initially hidden but shown before the page is loaded', {
    show: false
  }, async () => {
    w.show();
    await load();
    const state = await getVisibilityState();
    expect(state).to.equal('visible');
  });

  itWithOptions('should be hidden when the window is initially shown but hidden before the page is loaded', {
    show: true
  }, async () => {
    // TODO(MarshallOfSound): Figure out if we can work around this 1 tick issue for users
    if (process.platform === 'darwin') {
      // Wait for a tick, the window being "shown" takes 1 tick on macOS
      await delay(0);
    }
    w.hide();
    await load();
    const state = await getVisibilityState();
    expect(state).to.equal('hidden');
  });

  itWithOptions('should be toggle between visible and hidden as the window is hidden and shown', {}, async () => {
    await load();
    expect(await getVisibilityState()).to.equal('visible');
    await emittedOnce(ipcMain, 'visibility-change', () => w.hide());
    expect(await getVisibilityState()).to.equal('hidden');
    await emittedOnce(ipcMain, 'visibility-change', () => w.show());
    expect(await getVisibilityState()).to.equal('visible');
  });

  itWithOptions('should become hidden when a window is minimized', {}, async () => {
    await load();
    expect(await getVisibilityState()).to.equal('visible');
    await emittedOnce(ipcMain, 'visibility-change', () => w.minimize());
    expect(await getVisibilityState()).to.equal('hidden');
  });

  itWithOptions('should become visible when a window is restored', {}, async () => {
    await load();
    expect(await getVisibilityState()).to.equal('visible');
    await emittedOnce(ipcMain, 'visibility-change', () => w.minimize());
    await emittedOnce(ipcMain, 'visibility-change', () => w.restore());
    expect(await getVisibilityState()).to.equal('visible');
  });

  describe('on platforms that support occlusion detection', () => {
    let child: cp.ChildProcess;

    before(function () {
      if (process.platform !== 'darwin') this.skip();
    });

    const makeOtherWindow = (opts: { x: number; y: number; width: number; height: number; }) => {
      child = cp.spawn(process.execPath, [path.resolve(__dirname, 'fixtures', 'chromium', 'other-window.js'), `${opts.x}`, `${opts.y}`, `${opts.width}`, `${opts.height}`]);
      return new Promise(resolve => {
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

    itWithOptions('should be visible when two windows are on screen', {
      x: 0,
      y: 0,
      width: 200,
      height: 200
    }, async () => {
      await makeOtherWindow({
        x: 200,
        y: 0,
        width: 200,
        height: 200
      });
      await load();
      const state = await getVisibilityState();
      expect(state).to.equal('visible');
    });

    itWithOptions('should be visible when two windows are on screen that overlap partially', {
      x: 50,
      y: 50,
      width: 150,
      height: 150
    }, async () => {
      await makeOtherWindow({
        x: 100,
        y: 0,
        width: 200,
        height: 200
      });
      await load();
      const state = await getVisibilityState();
      expect(state).to.equal('visible');
    });

    itWithOptions('should be hidden when a second window completely conceals the current window', {
      x: 50,
      y: 50,
      width: 50,
      height: 50
    }, async function () {
      this.timeout(240000);
      await load();
      await emittedOnce(ipcMain, 'visibility-change', async () => {
        await makeOtherWindow({
          x: 0,
          y: 0,
          width: 300,
          height: 300
        });
      });
      const state = await getVisibilityState();
      expect(state).to.equal('hidden');
    });
  });
});
