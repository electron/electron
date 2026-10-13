import { app, BrowserWindow } from 'electron/main';

import { expect } from 'chai';

import * as cp from 'node:child_process';
import { once } from 'node:events';
import * as fs from 'node:fs';
import * as os from 'node:os';
import * as path from 'node:path';

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

type OnScreenWindow = { pid: number; width: number; height: number };

// The windows the window server has on screen, with the process that owns
// each. Read with JXA so that the answer comes from outside Electron.
function onScreenWindows(): OnScreenWindow[] {
  const script = `
    ObjC.import('CoreGraphics');
    const list = ObjC.deepUnwrap(ObjC.castRefToObject(
      $.CGWindowListCopyWindowInfo($.kCGWindowListOptionOnScreenOnly, 0)));
    JSON.stringify(list.map((w) => ({
      pid: w.kCGWindowOwnerPID,
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

  before(() => {
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

  after(() => {
    app.stopAppShimServer();
    fs.rmSync(tmpDir, { recursive: true, force: true });
  });

  afterEach(async () => {
    await closeAllWindows();
    if (shimProcess && shimProcess.exitCode === null) {
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
    expect(() => app.startAppShimServer({ name: serverName })).to.throw(/already started/);
  });

  it('connects a shim and says which process it is', async () => {
    const shim = await launchShim();
    expect(shim.isConnected()).to.be.true();
    expect(shim.pid).to.equal(shimProcess!.pid);
    expect(shim.bundleId).to.equal(SHIM_BUNDLE_ID);
    expect(fs.realpathSync(shim.bundlePath)).to.equal(fs.realpathSync(bundlePath));
  });

  it('checks the shim against a code requirement', async () => {
    const shim = await launchShim();
    expect(shim.satisfiesCodeRequirement(`identifier "${SHIM_BUNDLE_ID}.not-this"`)).to.be.false();
    expect(() => shim.satisfiesCodeRequirement('not a requirement')).to.throw(/Invalid code requirement/);
  });

  it('creates the window in the shim process', async () => {
    const shim = await launchShim();
    const size = { width: 432, height: 321 };
    const w = new BrowserWindow({ appShim: shim, ...size });
    await w.loadURL('data:text/html,<title>in a shim</title><body style="background:teal">');
    expect(await w.webContents.executeJavaScript('document.title')).to.equal('in a shim');

    const sameSize = (win: OnScreenWindow) => win.width === size.width && win.height === size.height;
    await waitUntil(() => onScreenWindows().some((win) => win.pid === shim.pid && sameSize(win)));
    // Its stand-in in this process is never on screen.
    expect(onScreenWindows().some((win) => win.pid === process.pid && sameSize(win))).to.be.false();
  });

  it('reports the bounds and state of a window in a shim', async () => {
    const shim = await launchShim();
    const w = new BrowserWindow({ appShim: shim, x: 80, y: 90, width: 400, height: 300 });
    // The shim's process shows the window, and says so afterwards.
    await once(w, 'show');
    expect(w.isVisible()).to.be.true();
    expect(w.getBounds()).to.deep.equal({ x: 80, y: 90, width: 400, height: 300 });

    const resized = once(w, 'resize');
    w.setSize(500, 350);
    await resized;
    expect(w.getSize()).to.deep.equal([500, 350]);

    const hidden = once(w, 'hide');
    w.hide();
    await hidden;
    expect(w.isVisible()).to.be.false();
  });

  it('lets the close event keep a window in a shim open', async () => {
    const shim = await launchShim();
    const w = new BrowserWindow({ appShim: shim });
    w.once('close', (event) => event.preventDefault());
    w.close();
    expect(w.isDestroyed()).to.be.false();

    const closed = once(w, 'closed');
    w.close();
    await closed;
    expect(w.isDestroyed()).to.be.true();
    expect(shim.isConnected()).to.be.true();
  });

  it('closes the windows of a shim that goes away', async () => {
    const shim = await launchShim();
    const w = new BrowserWindow({ appShim: shim });
    const closed = once(w, 'closed');
    const disconnected = once(shim, 'disconnected');
    shimProcess!.kill('SIGKILL');
    await Promise.all([closed, disconnected]);
    expect(w.isDestroyed()).to.be.true();
    expect(shim.isConnected()).to.be.false();
    expect(() => new BrowserWindow({ appShim: shim })).to.throw(/disconnected/);
  });

  it('ends the shim process on quit()', async () => {
    const shim = await launchShim();
    const exited = once(shimProcess!, 'exit');
    shim.quit();
    await exited;
    expect(shim.isConnected()).to.be.false();
  });

  it('rejects an appShim that is not an AppShimHost', () => {
    expect(() => new BrowserWindow({ appShim: {} as any })).to.throw(/must be an AppShimHost/);
  });
});
