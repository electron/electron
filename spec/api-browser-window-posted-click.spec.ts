// Clicks that another process posts straight to an app with CGEventPostToPid()
// (see spec/fixtures/native-addon/mouse-input). The window server does not
// route them, so unlike a click at an event tap they do not activate the app.
// The app under test is a second Electron process, so that this one is the
// other process. Its main process logs activations and mouse downs, and there
// are no retries: a click that only sometimes gets through is a failure.
import { app } from 'electron/main';

import { afterEach, beforeAll, beforeEach, expect, it } from 'vitest';

import { once } from 'node:events';
import { createRequire } from 'node:module';

import { ifdescribe, startRemoteControlApp, waitUntil } from './lib/spec-helpers.ts';

const require = createRequire(import.meta.url);

type MouseInput = {
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

    let canPostEvents = true;
    beforeAll(() => {
      mouse = require('@electron-ci/mouse-input');
      canPostEvents = !!process.env.CI || !!mouse.getDiagnostics().postEventAccess;
    });
    beforeEach((ctx) => {
      if (!canPostEvents) ctx.skip();
    });

    beforeEach(async () => {
      rc = await startRemoteControlApp();
      await rc.remotely(() => {
        const { app } = require('electron');
        const log: string[] = ((globalThis as any).log = []);
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
      expect(await log()).toEqual(['mouseDown main']);
    });

    it('reach the page without activating the app', async () => {
      const w = await openWindow('main');
      await sendToBackground();
      postClick(w);
      await logged('mouseDown main');
      expect(await log()).toEqual(['mouseDown main']);
    });
  }
);
