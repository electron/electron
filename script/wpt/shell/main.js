// Host app for running web-platform-tests against Electron.
//
// wptrunner starts the Electron binary through Electron's ChromeDriver with
// `--app=<this directory>`. ChromeDriver attaches over CDP, treats the first
// BrowserWindow's webContents like a Chrome tab and navigates it itself, so
// this app only opens one window and otherwise stays out of the way. The
// Chromium switches wptrunner adds reach Chromium through the process command
// line; nothing needs forwarding here.
//
// The aim is to behave like Chrome does without any UI, so that differences
// in results come from Electron rather than from this app:
//
//   - One window with contextIsolation and sandbox on, no nodeIntegration and
//     no preload, and an 800x600 content area (the size wptrunner wants for
//     reftests).
//   - window.open is allowed, with the same webPreferences. wptrunner opens
//     its test windows this way.
//   - window-all-closed is ignored: ChromeDriver decides when to quit.
//   - Certificate errors are accepted for the wpt hosts only, as a fallback
//     for --ignore-certificate-errors-spki-list.
//   - Permissions follow Chrome's defaults. Electron grants every request and
//     check when no handler is set; Chrome without UI leaves permissions at
//     "prompt" and denies the requests that would need one. Electron's check
//     handler can only answer granted or denied, so checks report "denied"
//     where Chrome reports "prompt". Overrides from CDP Browser.setPermission
//     (test_driver.set_permission) are applied before these handlers run.

const { app, BrowserWindow, session } = require('electron');

const WPT_HOST_RE =
  /^(?:[^.]+\.)*(?:web-platform\.test|not-web-platform\.test|xn--lve-6lad\.(?:web-platform|not-web-platform)\.test|localhost|127\.0\.0\.1)$/;

// Permissions Chrome grants without showing a prompt.
const GRANTED_WITHOUT_PROMPT = new Set(['clipboard-sanitized-write', 'fullscreen', 'pointerLock', 'keyboardLock']);

const webPreferences = {
  contextIsolation: true,
  nodeIntegration: false,
  sandbox: true,
  backgroundThrottling: false
};

function log(...args) {
  if (process.env.ELECTRON_WPT_SHELL_LOG) console.error('[wpt-shell]', ...args);
}

function setupSession(ses) {
  ses.setPermissionRequestHandler((wc, permission, callback, details) => {
    const granted = GRANTED_WITHOUT_PROMPT.has(permission);
    log('request', permission, details && details.requestingUrl, granted);
    callback(granted);
  });
  ses.setPermissionCheckHandler((wc, permission, origin) => {
    const granted = GRANTED_WITHOUT_PROMPT.has(permission);
    log('check', permission, origin, granted);
    return granted;
  });
}

app.on('certificate-error', (event, webContents, url, error, certificate, callback) => {
  let host = '';
  try {
    host = new URL(url).hostname;
  } catch {}
  const trusted = WPT_HOST_RE.test(host);
  log('certificate-error', url, error, trusted);
  if (trusted) event.preventDefault();
  callback(trusted);
});

app.on('web-contents-created', (_event, wc) => {
  wc.setWindowOpenHandler(({ features }) => {
    // The size and position a page asks for must reach the new window as they
    // do in Chrome, where width and height are the content size. Popups that
    // do not ask for a size get the same 800x600 content area as the main
    // window.
    const requested = (features || '').toLowerCase();
    const options = { useContentSize: true, webPreferences };
    if (!/(^|[\s,])(width|innerwidth)\s*=/.test(requested)) options.width = 800;
    if (!/(^|[\s,])(height|innerheight)\s*=/.test(requested)) options.height = 600;
    return { action: 'allow', overrideBrowserWindowOptions: options };
  });
});

app.on('window-all-closed', () => {});

app.whenReady().then(() => {
  log('argv', JSON.stringify(process.argv));
  setupSession(session.defaultSession);
  app.on('session-created', setupSession);
  const win = new BrowserWindow({
    width: 800,
    height: 600,
    useContentSize: true,
    show: true,
    webPreferences
  });
  win.loadURL('about:blank');
});
