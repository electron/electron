// Clicks that another process posts straight to an app with CGEventPostToPid()
// (see spec/fixtures/native-addon/mouse-input). The window server does not
// route them, so unlike a click at an event tap they do not activate the app.
// The app under test is a second Electron process, so that this one is the
// other process. Its main process logs activations and mouse downs in the order
// AppKit hands them over, so a later click shows that an earlier one is lost.
// There are no retries: a click that only sometimes gets through is a failure.
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
const POINT = { x: BOUNDS.x + 200, y: BOUNDS.y + 200 };

ifdescribe(process.platform === 'darwin' && !process.env.ELECTRON_SKIP_NATIVE_MODULE_TESTS)(
  'clicks posted to a background app',
  { tags: ['serial'], retry: 0 },
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
      await rc.remotely(() => {
        const { app } = require('electron');
        const log: string[] = ((globalThis as any).log = []);
        (globalThis as any).windows = {};
        app.on('did-become-active', () => log.push('active'));
        app.on('did-resign-active', () => log.push('inactive'));
      });
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
          const w = new BrowserWindow({ ...options, show: false });
          (globalThis as any).windows[name] = w;
          w.webContents.on('input-event', (_: unknown, input: Electron.InputEvent) => {
            if (input.type === 'mouseDown') (globalThis as any).log.push(`mouseDown ${name}`);
          });
          await w.loadURL('about:blank');
          w.showInactive();
          return Number(w.getMediaSourceId().split(':')[1]);
        },
        name,
        { ...BOUNDS, frame: false, alwaysOnTop: true, ...options }
      );
    const log = (): Promise<string[]> => rc.remotely(() => (globalThis as any).log);
    const logged = (entry: string) => waitUntil(async () => (await log()).includes(entry));
    const sendToBackground = async () => {
      app.focus({ steal: true });
      await waitUntil(() => rc.remotely(() => !require('electron').app.isActive()));
      await rc.remotely(() => {
        (globalThis as any).log.length = 0;
      });
    };
    const postClick = (windowNumber: number) =>
      mouse.postClickToWindow(rc.process.pid!, windowNumber, POINT.x, POINT.y);

    it('reach a window that accepts the first mouse', async () => {
      const w = await openWindow('main', { acceptFirstMouse: true });
      await sendToBackground();
      postClick(w);
      await logged('mouseDown main');
      expect(await log()).to.deep.equal(['mouseDown main']);
    });

    it('reach the page without activating the app', async () => {
      const w = await openWindow('main');
      await sendToBackground();
      postClick(w);
      await logged('mouseDown main');
      expect(await log()).to.deep.equal(['mouseDown main']);
    });

    it('do not reach a window that ignores mouse events', async () => {
      const w = await openWindow('main');
      const overlay = await openWindow('overlay', { transparent: true });
      await rc.remotely(() => (globalThis as any).windows.overlay.setIgnoreMouseEvents(true));
      await sendToBackground();
      postClick(overlay);
      postClick(w);
      await logged('mouseDown main');
      expect(await log()).to.deep.equal(['mouseDown main']);
    });

    it('leave a click that the window server routes to only activate the app', async () => {
      await openWindow('main');
      await sendToBackground();
      await mouse.move(POINT.x, POINT.y);
      await waitUntil(() => {
        const { x, y } = mouse.getCursorPos();
        return x === POINT.x && y === POINT.y;
      });
      await mouse.click();
      await logged('active');
      await mouse.click();
      await logged('mouseDown main');
      expect(await log()).to.deep.equal(['active', 'mouseDown main']);
    });
  }
);
