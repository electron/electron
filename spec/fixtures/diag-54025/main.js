// Probe for https://github.com/electron/electron/issues/54025:
// `ready-to-show` never fires for a `show: false` window that has a
// `titleBarOverlay` (Windows, 44.4.x).
//
// Usage: electron spec/fixtures/diag-54025 [--show] [--no-overlay]
//   [--no-hidden-style] [--bg-throttle] [--tag=<label>]
// Prints timestamped events and a final single-line `RESULT {...}` JSON.
const { app, BrowserWindow } = require('electron');
const path = require('node:path');
const os = require('node:os');
const fs = require('node:fs');

const argv = process.argv.slice(2);
const has = (f) => argv.includes(f);
const tag = (argv.find((a) => a.startsWith('--tag=')) || '--tag=').slice(6);
const opts = {
  show: has('--show'),
  overlay: !has('--no-overlay'),
  hiddenStyle: !has('--no-hidden-style'),
  bgThrottle: has('--bg-throttle')
};

const userData = fs.mkdtempSync(path.join(os.tmpdir(), 'diag-54025-'));
app.setPath('userData', userData);

const t0 = Date.now();
const events = {};
const log = (name, extra) => {
  const t = Date.now() - t0;
  if (!(name in events)) events[name] = t;
  console.log(`[diag-54025] +${t}ms ${name}${extra ? ' ' + extra : ''}`);
};

const withTimeout = (p, ms, fallback) =>
  Promise.race([p, new Promise((resolve) => setTimeout(() => resolve(fallback), ms))]);

app.whenReady().then(async () => {
  log('app-ready');
  const winOpts = {
    width: 900,
    height: 600,
    show: opts.show,
    webPreferences: { backgroundThrottling: opts.bgThrottle, sandbox: true }
  };
  if (opts.hiddenStyle) winOpts.titleBarStyle = 'hidden';
  if (opts.overlay) winOpts.titleBarOverlay = { color: '#202020', symbolColor: '#f0f0f0', height: 36 };
  const w = new BrowserWindow(winOpts);
  log('window-created');

  let readyToShow = false;
  w.once('ready-to-show', () => {
    readyToShow = true;
    log('ready-to-show');
  });
  w.once('show', () => log('show'));
  w.webContents.once('did-start-loading', () => log('did-start-loading'));
  w.webContents.once('dom-ready', () => log('dom-ready'));
  w.webContents.once('did-finish-load', () => log('did-finish-load'));
  w.webContents.on('render-process-gone', (_e, d) => log('render-process-gone', JSON.stringify(d)));

  const html =
    '<!doctype html><title>t</title><body style="margin:0;background:#123456;color:#fff">' +
    '<h1>diag 54025</h1><p>' +
    'lorem ipsum '.repeat(300) +
    '</p></body>';
  w.loadURL('data:text/html;charset=utf-8,' + encodeURIComponent(html)).then(
    () => log('loadURL-resolved'),
    (e) => log('loadURL-rejected', String(e))
  );

  // Give first paint a generous window.
  await new Promise((resolve) => setTimeout(resolve, 4000));

  // Probe the page without depending on rendering: does rAF run (i.e. are
  // main frames being produced), and what WCO geometry does the page see?
  const probe = async (label) => {
    const js = `(() => new Promise((resolve) => {
      const info = {
        inner: [innerWidth, innerHeight],
        visibility: document.visibilityState,
        wcoVisible: navigator.windowControlsOverlay ? navigator.windowControlsOverlay.visible : null,
        wcoRect: navigator.windowControlsOverlay ? (() => { const r = navigator.windowControlsOverlay.getTitlebarAreaRect(); return [r.x, r.y, r.width, r.height]; })() : null,
        raf: false
      };
      const done = () => resolve(JSON.stringify(info));
      const timer = setTimeout(done, 1500);
      requestAnimationFrame(() => { info.raf = true; clearTimeout(timer); done(); });
    }))()`;
    const r = await withTimeout(
      w.webContents.executeJavaScript(js).catch((e) => 'ERR ' + e),
      2500,
      'TIMEOUT'
    );
    log('probe-' + label, r);
    return r;
  };
  const probeHidden = await probe('hidden');

  // Does showing the window un-stick it?
  const rtsBeforeShow = readyToShow;
  if (!opts.show) {
    w.show();
    log('called-show');
    await new Promise((resolve) => setTimeout(resolve, 2500));
  }
  const probeShown = opts.show ? null : await probe('shown');

  const result = {
    tag,
    electron: process.versions.electron,
    chrome: process.versions.chrome,
    platform: process.platform,
    arch: process.arch,
    osRelease: os.release(),
    diagEnv: process.env.ELECTRON_DIAG_WCO || '',
    opts,
    readyToShowWhileHidden: opts.show ? null : rtsBeforeShow,
    readyToShowAfterShow: opts.show ? null : readyToShow,
    readyToShow,
    events,
    probeHidden: (() => {
      try {
        return JSON.parse(probeHidden);
      } catch {
        return probeHidden;
      }
    })(),
    probeShown: (() => {
      try {
        return JSON.parse(probeShown);
      } catch {
        return probeShown;
      }
    })()
  };
  console.log('RESULT ' + JSON.stringify(result));
  w.destroy();
  try {
    fs.rmSync(userData, { recursive: true, force: true });
  } catch {}
  app.exit(0);
});

setTimeout(() => {
  console.log('RESULT ' + JSON.stringify({ tag, error: 'global-timeout', events }));
  app.exit(2);
}, 20000);
