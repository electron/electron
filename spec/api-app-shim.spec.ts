import { app, BaseWindow, BrowserWindow, dialog, Menu } from 'electron/main';

import { afterAll, afterEach, beforeAll, expect, it } from 'vitest';

import * as cp from 'node:child_process';
import { once } from 'node:events';
import * as fs from 'node:fs';
import * as os from 'node:os';
import * as path from 'node:path';
import { setTimeout } from 'node:timers/promises';

import { ifdescribe, waitUntil } from './lib/spec-helpers.ts';
import { closeAllWindows } from './lib/window-helpers.ts';

// Electron.app, which the specs run in and which hosts the shims.
const hostAppPath = path.resolve(process.execPath, '..', '..', '..');
const loaderPath = path.join(
  hostAppPath,
  'Contents',
  'Frameworks',
  'Electron Framework.framework',
  'Helpers',
  'app_shim_loader'
);

const SHIM_NAME = 'Spec Shim';
const SHIM_BUNDLE_ID = 'org.electronjs.spec.app-shim';

type OnScreenWindow = { pid: number; x: number; y: number; width: number; height: number };

// The windows the window server has on screen, with the process that owns
// each. Read with JXA so that the answer comes from outside Electron.
function onScreenWindows(): OnScreenWindow[] {
  const script = `
    ObjC.import('CoreGraphics');
    const list = ObjC.deepUnwrap(ObjC.castRefToObject(
      $.CGWindowListCopyWindowInfo($.kCGWindowListOptionOnScreenOnly, 0)));
    JSON.stringify(list.map((w) => ({
      pid: w.kCGWindowOwnerPID,
      x: w.kCGWindowBounds.X,
      y: w.kCGWindowBounds.Y,
      width: w.kCGWindowBounds.Width,
      height: w.kCGWindowBounds.Height
    })));`;
  const { stdout } = cp.spawnSync('osascript', ['-l', 'JavaScript', '-e', script], { encoding: 'utf8' });
  return JSON.parse(stdout);
}

ifdescribe(process.platform === 'darwin' && !process.mas)('app shims', () => {
  // Another Electron running specs must not get this one's shims.
  const serverName = `spec-${process.pid}`;
  let tmpDir: string;
  let bundlePath: string;
  let shimProcess: cp.ChildProcess | null = null;

  beforeAll(() => {
    tmpDir = fs.mkdtempSync(path.join(os.tmpdir(), 'electron-app-shim-'));
    bundlePath = path.join(tmpDir, `${SHIM_NAME}.app`);
    const macOSDir = path.join(bundlePath, 'Contents', 'MacOS');
    fs.mkdirSync(macOSDir, { recursive: true });
    fs.copyFileSync(loaderPath, path.join(macOSDir, SHIM_NAME));
    fs.writeFileSync(
      path.join(bundlePath, 'Contents', 'Info.plist'),
      `<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
  <key>CFBundleExecutable</key><string>${SHIM_NAME}</string>
  <key>CFBundleIdentifier</key><string>${SHIM_BUNDLE_ID}</string>
  <key>CFBundleName</key><string>${SHIM_NAME}</string>
  <key>CFBundlePackageType</key><string>APPL</string>
  <key>NSHighResolutionCapable</key><true/>
  <key>ElectronAppShimHostPath</key><string>${hostAppPath}</string>
  <key>ElectronAppShimServerName</key><string>${serverName}</string>
</dict>
</plist>
`
    );
    app.startAppShimServer({ name: serverName });
  });

  afterAll(() => {
    app.stopAppShimServer();
    fs.rmSync(tmpDir, { recursive: true, force: true });
  });

  afterEach(async () => {
    await closeAllWindows();
    if (shimProcess && shimProcess.exitCode === null && shimProcess.signalCode === null) {
      const exited = once(shimProcess, 'exit');
      shimProcess.kill('SIGKILL');
      await exited;
    }
    shimProcess = null;
  });

  async function launchShim(): Promise<Electron.AppShimHost> {
    const connected = once(app, 'app-shim-connected');
    shimProcess = cp.spawn(path.join(bundlePath, 'Contents', 'MacOS', SHIM_NAME), { stdio: 'inherit' });
    const [, shim] = await connected;
    return shim;
  }

  it('throws when the server is started twice', () => {
    expect(() => app.startAppShimServer({ name: serverName })).toThrow(/already started/);
  });

  it('connects a shim and says which process it is', async () => {
    const shim = await launchShim();
    expect(shim.isConnected()).toBe(true);
    expect(shim.pid).toBe(shimProcess!.pid);
    expect(shim.bundleId).toBe(SHIM_BUNDLE_ID);
    expect(fs.realpathSync(shim.bundlePath)).toBe(fs.realpathSync(bundlePath));
  });

  it('checks the shim against a code requirement', async () => {
    const shim = await launchShim();
    expect(shim.satisfiesCodeRequirement(`identifier "${SHIM_BUNDLE_ID}.not-this"`)).toBe(false);
    expect(() => shim.satisfiesCodeRequirement('not a requirement')).toThrow(/Invalid code requirement/);
  });

  it('creates the window in the shim process', async () => {
    const shim = await launchShim();
    const size = { width: 432, height: 321 };
    const w = new BrowserWindow({ appShim: shim, ...size });
    await w.loadURL('data:text/html,<title>in a shim</title><body style="background:teal">');
    expect(await w.webContents.executeJavaScript('document.title')).toBe('in a shim');

    const sameSize = (win: OnScreenWindow) => win.width === size.width && win.height === size.height;
    await waitUntil(() => onScreenWindows().some((win) => win.pid === shim.pid && sameSize(win)));
    // Its stand-in in this process is never on screen.
    expect(onScreenWindows().some((win) => win.pid === process.pid && sameSize(win))).toBe(false);
  });

  it('reports the bounds and state of a window in a shim', async () => {
    const shim = await launchShim();
    const w = new BrowserWindow({ appShim: shim, x: 80, y: 90, width: 400, height: 300 });
    // The shim's process shows the window, and says so afterwards.
    await once(w, 'show');
    expect(w.isVisible()).toBe(true);
    expect(w.getBounds()).toEqual({ x: 80, y: 90, width: 400, height: 300 });

    w.setSize(500, 350);
    expect(w.getSize()).toEqual([500, 350]);
    // What the shim's process did with it, according to the window server.
    await waitUntil(() =>
      onScreenWindows().some((win) => win.pid === shim.pid && win.width === 500 && win.height === 350)
    );

    const hidden = once(w, 'hide');
    w.hide();
    await hidden;
    expect(w.isVisible()).toBe(false);
  });

  it('lets the close event keep a window in a shim open', async () => {
    const shim = await launchShim();
    // A BaseWindow closes inside close() when nothing prevents it.
    const w = new BaseWindow({ appShim: shim });
    let closedEmitted = false;
    w.once('closed', () => {
      closedEmitted = true;
    });
    w.once('close', (event) => event.preventDefault());
    w.close();
    await setTimeout(100);
    expect(closedEmitted).toBe(false);
    expect(w.isDestroyed()).toBe(false);

    w.close();
    expect(closedEmitted).toBe(true);
    expect(shim.isConnected()).toBe(true);
  });

  it('sizes a framed window by its content when asked to', async () => {
    const shim = await launchShim();
    const w = new BaseWindow({ appShim: shim, width: 400, height: 300, useContentSize: true });
    expect(w.getContentSize()).toEqual([400, 300]);
    const titleBarHeight = w.getSize()[1] - 300;
    expect(titleBarHeight).toBeGreaterThan(0);

    // The limits are the window's, title bar included.
    w.setMinimumSize(300, 200);
    w.setSize(100, 100);
    await waitUntil(() =>
      onScreenWindows().some((win) => win.pid === shim.pid && win.width === 300 && win.height === 200)
    );
  });

  it('survives a window being destroyed by its own show listener', async () => {
    const shim = await launchShim();
    const w = new BaseWindow({ appShim: shim });
    const closed = once(w, 'closed');
    w.once('show', () => w.destroy());
    await closed;
    expect(w.isDestroyed()).toBe(true);
    expect(shim.isConnected()).toBe(true);
  });

  it('survives the shim being quit by a show listener of its window', async () => {
    const shim = await launchShim();
    const exited = once(shimProcess!, 'exit');
    const w = new BaseWindow({ appShim: shim });
    const closed = once(w, 'closed');
    // What the listener saw, checked below: a failed expectation inside a
    // listener is an uncaught exception, not a failed test.
    let connectedAfterQuit: boolean | undefined;
    let errorFromNewWindow: unknown;
    w.once('show', () => {
      shim.quit();
      connectedAfterQuit = shim.isConnected();
      try {
        const unexpected = new BaseWindow({ appShim: shim });
        unexpected.destroy();
      } catch (error) {
        errorFromNewWindow = error;
      }
    });
    await Promise.all([closed, exited, once(shim, 'disconnected')]);
    expect(connectedAfterQuit).toBe(false);
    expect(errorFromNewWindow).toBeInstanceOf(Error);
    expect((errorFromNewWindow as Error).message).toMatch(/disconnected/);
  });

  it('restores from the Dock without leaving the maximized state', async () => {
    const shim = await launchShim();
    const w = new BaseWindow({ appShim: shim, width: 400, height: 300 });
    await once(w, 'show');
    const maximized = once(w, 'maximize');
    w.maximize();
    await maximized;
    const minimized = once(w, 'minimize');
    w.minimize();
    await minimized;
    const restored = once(w, 'restore');
    w.restore();
    await restored;
    expect(w.isMaximized()).toBe(true);
  });

  it('does not announce a shim after the server is stopped', async () => {
    app.stopAppShimServer();
    try {
      let announced = false;
      const onConnected = () => {
        announced = true;
      };
      app.once('app-shim-connected', onConnected);
      shimProcess = cp.spawn(path.join(bundlePath, 'Contents', 'MacOS', SHIM_NAME), { stdio: 'inherit' });
      await setTimeout(1500);
      app.removeListener('app-shim-connected', onConnected);
      expect(announced).toBe(false);
    } finally {
      app.startAppShimServer({ name: serverName });
    }
  });

  it('rejects options that are not what startAppShimServer takes', () => {
    app.stopAppShimServer();
    try {
      expect(() => app.startAppShimServer({ name: 123 as any })).toThrow(/name must be a string/);
      expect(() => app.startAppShimServer({ name: 'not valid!' })).toThrow(/may only hold/);
    } finally {
      app.startAppShimServer({ name: serverName });
    }
  });

  it('shows a message box as a sheet in the shim process', async () => {
    const shim = await launchShim();
    const w = new BaseWindow({ appShim: shim, width: 400, height: 300 });
    await once(w, 'show');
    const windowsOfShim = () => onScreenWindows().filter((win) => win.pid === shim.pid).length;
    await waitUntil(() => windowsOfShim() === 1);

    const controller = new AbortController();
    const result = dialog.showMessageBox(w, {
      message: 'A sheet',
      buttons: ['OK', 'Cancel'],
      cancelId: 1,
      signal: controller.signal
    });
    // The sheet is a second window of the shim's process, not of this one.
    await waitUntil(() => windowsOfShim() === 2);
    controller.abort();
    expect((await result).response).toBe(1);
    await waitUntil(() => windowsOfShim() === 1);
  });

  it('answers a dialog with "cancelled" when its shim goes away', async () => {
    const shim = await launchShim();
    const w = new BaseWindow({ appShim: shim, width: 400, height: 300 });
    await once(w, 'show');
    const result = dialog.showOpenDialog(w, { properties: ['openFile'] });
    await waitUntil(() => onScreenWindows().filter((win) => win.pid === shim.pid).length >= 2);
    shimProcess!.kill('SIGKILL');
    expect(await result).toEqual({ canceled: true, filePaths: [] });
  });

  it('takes a menu of its own, or null, and nothing else', async () => {
    const shim = await launchShim();
    const menu = Menu.buildFromTemplate([{ role: 'appMenu' }, { label: 'Tools', submenu: [{ label: 'Ping' }] }]);
    expect(() => shim.setApplicationMenu(menu)).not.toThrow();
    expect(() => shim.setApplicationMenu(null)).not.toThrow();
    expect(() => shim.setApplicationMenu('menu' as any)).toThrow(/Must pass a Menu or null/);
    expect(() => (shim as any).setApplicationMenu()).toThrow(/Must pass a Menu or null/);
    expect(shim.isConnected()).toBe(true);
  });

  it('gives a window with a hidden title bar its whole frame as content', async () => {
    const shim = await launchShim();
    const w = new BaseWindow({ appShim: shim, width: 400, height: 300, titleBarStyle: 'hidden' });
    await once(w, 'show');
    expect(w.getContentSize()).toEqual([400, 300]);
    await waitUntil(() =>
      onScreenWindows().some((win) => win.pid === shim.pid && win.width === 400 && win.height === 300)
    );
    // The window buttons are the shim's to move and hide.
    w.setWindowButtonPosition({ x: 20, y: 20 });
    expect(w.getWindowButtonPosition()).toEqual({ x: 20, y: 20 });
    w.setWindowButtonVisibility(false);
    w.setVibrancy('sidebar');
    w.setVibrancy(null);
    await setTimeout(200);
    expect(shim.isConnected()).toBe(true);
  });

  it('tabs two windows of one shim together, and no others', async () => {
    const shim = await launchShim();
    const windowsOfShim = () => onScreenWindows().filter((win) => win.pid === shim.pid).length;
    const a = new BaseWindow({ appShim: shim, width: 400, height: 300, tabbingIdentifier: 'group' });
    const b = new BaseWindow({ appShim: shim, width: 400, height: 300, tabbingIdentifier: 'group' });
    expect(a.tabbingIdentifier).toBe('group');
    await waitUntil(() => windowsOfShim() === 2);
    a.addTabbedWindow(b);
    // One window on screen, with the other as a tab in it.
    await waitUntil(() => windowsOfShim() === 1);

    const local = new BaseWindow({ show: false, tabbingIdentifier: 'group' });
    expect(() => a.addTabbedWindow(local)).toThrow(/same app/);
    expect(() => local.addTabbedWindow(a)).toThrow(/same app/);
    local.destroy();
  });

  it('does not take a window in a shim as a parent', async () => {
    const shim = await launchShim();
    const w = new BaseWindow({ appShim: shim });
    expect(() => new BaseWindow({ parent: w })).toThrow(/can't be a parent window/);
    const other = new BaseWindow({ show: false });
    expect(() => other.setParentWindow(w)).toThrow(/can't be a parent window/);
    // Nor does it take one.
    expect(() => new BaseWindow({ appShim: shim, parent: other })).toThrow(/can't have a parent window/);
    expect(() => w.setParentWindow(other)).toThrow(/can't have a parent window/);
    expect(w.getParentWindow()).toBeNull();
    other.destroy();
  });

  it('closes the windows of a shim that goes away', async () => {
    const shim = await launchShim();
    const w = new BrowserWindow({ appShim: shim });
    const closed = once(w, 'closed');
    const disconnected = once(shim, 'disconnected');
    shimProcess!.kill('SIGKILL');
    await Promise.all([closed, disconnected]);
    expect(w.isDestroyed()).toBe(true);
    expect(shim.isConnected()).toBe(false);
    expect(() => new BrowserWindow({ appShim: shim })).toThrow(/disconnected/);
  });

  it('ends the shim process on quit()', async () => {
    const shim = await launchShim();
    const exited = once(shimProcess!, 'exit');
    shim.quit();
    await exited;
    expect(shim.isConnected()).toBe(false);
  });

  it('rejects an appShim that is not an AppShimHost', () => {
    expect(() => new BrowserWindow({ appShim: {} as any })).toThrow(/must be an AppShimHost/);
  });
});
