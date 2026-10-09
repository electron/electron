// Clicks that another process posts straight to an app with CGEventPostToPid()
// (see spec/fixtures/native-addon/mouse-input). The window server does not
// route them, so unlike a click at an event tap they do not activate the app.
// The app under test is a second Electron process, so that this one is the
// other process. Mouse downs are counted in its main process, in the order
// AppKit hands them over, so a later click shows that an earlier one is lost.
import { app } from 'electron/main';

import { expect } from 'chai';

import { once } from 'node:events';
import { createRequire } from 'node:module';

import { ifdescribe, startRemoteControlApp, waitUntil } from './lib/spec-helpers.ts';

const require = createRequire(import.meta.url);

type MouseInput = {
  move(x: number, y: number): Promise<unknown>;
  click(): Promise<unknown>;
  getCursorPos(): { x?: number; y?: number };
  postClickToWindow(pid: number, windowNumber: number, x: number, y: number): unknown;
  getDiagnostics(): Record<string, unknown>;
};

const BOUNDS = { x: 100, y: 100, width: 400, height: 400 };
const POINT = { x: 200, y: 200 };

ifdescribe(process.platform === 'darwin' && !process.env.ELECTRON_SKIP_NATIVE_MODULE_TESTS)(
  'clicks posted to a background app',
  { tags: ['serial'] },
  () => {
    let mouse: MouseInput;
    let rc: Awaited<ReturnType<typeof startRemoteControlApp>>;
    let initialCursor: { x?: number; y?: number };

    before(function () {
      mouse = require('@electron-ci/mouse-input');
      if (!process.env.CI && !mouse.getDiagnostics().postEventAccess) this.skip();
      initialCursor = mouse.getCursorPos();
    });

    after(async () => {
      if (initialCursor?.x !== undefined && initialCursor?.y !== undefined) {
        await mouse.move(initialCursor.x, initialCursor.y);
      }
    });

    beforeEach(async () => {
      rc = await startRemoteControlApp();
    });

    afterEach(async () => {
      const exited = once(rc.process, 'exit');
      rc.process.kill('SIGKILL');
      await exited;
    });

    const openWindow = (name: string, options: Electron.BrowserWindowConstructorOptions = {}): Promise<number> =>
      rc.remotely(
        async (name: string, options: Electron.BrowserWindowConstructorOptions) => {
          const { BrowserWindow } = require('electron');
          const state = ((globalThis as any).state ??= { windows: {}, mouseDowns: {} });
          const w = new BrowserWindow({ ...options, show: false });
          state.windows[name] = w;
          state.mouseDowns[name] = 0;
          w.webContents.on('input-event', (_: unknown, input: Electron.InputEvent) => {
            if (input.type === 'mouseDown') state.mouseDowns[name]++;
          });
          await w.loadURL('about:blank');
          w.showInactive();
          return Number(w.getMediaSourceId().split(':')[1]);
        },
        name,
        { ...BOUNDS, frame: false, alwaysOnTop: true, ...options }
      );
    const mouseDowns = (name: string): Promise<number> =>
      rc.remotely((name: string) => (globalThis as any).state.mouseDowns[name], name);
    const isActive = (): Promise<boolean> => rc.remotely(() => require('electron').app.isActive());
    const sendToBackground = async () => {
      app.focus({ steal: true });
      await waitUntil(async () => !(await isActive()));
    };
    const postClick = (windowNumber: number) =>
      mouse.postClickToWindow(rc.process.pid!, windowNumber, POINT.x, BOUNDS.height - POINT.y);

    it('reach a window that accepts the first mouse', async () => {
      const w = await openWindow('main', { acceptFirstMouse: true });
      await sendToBackground();
      postClick(w);
      await waitUntil(async () => (await mouseDowns('main')) === 1);
      expect(await isActive()).to.be.false();
    });

    it('reach the page without activating the app', async () => {
      const w = await openWindow('main');
      await sendToBackground();
      postClick(w);
      await waitUntil(async () => (await mouseDowns('main')) === 1);
      expect(await isActive()).to.be.false();
    });

    it('do not reach a window that ignores mouse events', async () => {
      const w = await openWindow('main');
      const overlay = await openWindow('overlay', { transparent: true });
      await rc.remotely(() => (globalThis as any).state.windows.overlay.setIgnoreMouseEvents(true));
      await sendToBackground();
      postClick(overlay);
      postClick(w);
      await waitUntil(async () => (await mouseDowns('main')) === 1);
      expect(await mouseDowns('overlay')).to.equal(0);
    });

    it('leave a click that the window server routes to only activate the app', async () => {
      await openWindow('main');
      await sendToBackground();
      await mouse.move(BOUNDS.x + POINT.x, BOUNDS.y + POINT.y);
      await mouse.click();
      await waitUntil(isActive);
      await mouse.click();
      await waitUntil(async () => (await mouseDowns('main')) > 0);
      expect(await mouseDowns('main')).to.equal(1);
    });
  }
);
