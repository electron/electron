import * as path from 'path';
import { IPC_MESSAGES } from '@electron/internal/common/ipc-messages';

const Module = require('module');

// Make sure globals like "process" and "global" are always available in preload
// scripts even after they are deleted in "loaded" script.
//
// Note 1: We rely on a Node patch to actually pass "process" and "global" and
// other arguments to the wrapper.
//
// Note 2: Node introduced a new code path to use native code to wrap module
// code, which does not work with this hack. However by modifying the
// "Module.wrapper" we can force Node to use the old code path to wrap module
// code with JavaScript.
//
// Note 3: We provide the equivalent extra variables internally through the
// webpack ProvidePlugin in webpack.config.base.js.  If you add any extra
// variables to this wrapper please ensure to update that plugin as well.
Module.wrapper = [
  '(function (exports, require, module, __filename, __dirname, process, global, Buffer) { ' +
  // By running the code in a new closure, it would be possible for the module
  // code to override "process" and "Buffer" with local variables.
  'return function (exports, require, module, __filename, __dirname) { ',
  '\n}.call(this, exports, require, module, __filename, __dirname); });'
];

// We modified the original process.argv to let node.js load the
// init.js, we need to restore it here.
process.argv.splice(1, 1);

// Clear search paths.

require('../common/reset-search-paths');

// Import common settings.
require('@electron/internal/common/init');

// The global variable will be used by ipc for event dispatching
const v8Util = process._linkedBinding('electron_common_v8_util');

// Expose process.contextId
const contextId = v8Util.getHiddenValue<string>(global, 'contextId');
Object.defineProperty(process, 'contextId', { enumerable: true, value: contextId });

const { ipcRendererInternal } = require('@electron/internal/renderer/ipc-renderer-internal');
const ipcRenderer = require('@electron/internal/renderer/api/ipc-renderer').default;

v8Util.setHiddenValue(global, 'ipcNative', {
  onMessage (internal: boolean, channel: string, ports: any[], args: any[], senderId: number) {
    if (internal && senderId !== 0) {
      console.error(`Message ${channel} sent by unexpected WebContents (${senderId})`);
      return;
    }
    const sender = internal ? ipcRendererInternal : ipcRenderer;
    sender.emit(channel, { sender, senderId, ports }, ...args);
  }
});

// Use electron module after everything is ready.
const { webFrameInit } = require('@electron/internal/renderer/web-frame-init');
webFrameInit();

// Process command line arguments.
const { hasSwitch, getSwitchValue } = process._linkedBinding('electron_common_command_line');
const { mainFrame } = process._linkedBinding('electron_renderer_web_frame');

const contextIsolation = mainFrame.getWebPreference('contextIsolation');
const nodeIntegration = mainFrame.getWebPreference('nodeIntegration');
const webviewTag = mainFrame.getWebPreference('webviewTag');
const isHiddenPage = mainFrame.getWebPreference('hiddenPage');
const usesNativeWindowOpen = mainFrame.getWebPreference('nativeWindowOpen');
const preloadScript = mainFrame.getWebPreference('preload');
const preloadScripts = mainFrame.getWebPreference('preloadScripts');
const guestInstanceId = mainFrame.getWebPreference('guestInstanceId') || null;
const openerId = mainFrame.getWebPreference('openerId') || null;
const appPath = hasSwitch('app-path') ? getSwitchValue('app-path') : null;

// The webContents preload script is loaded after the session preload scripts.
if (preloadScript) {
  preloadScripts.push(preloadScript);
}

switch (window.location.protocol) {
  case 'devtools:': {
    // Override some inspector APIs.
    require('@electron/internal/renderer/inspector');
    break;
  }
  case 'chrome-extension:': {
    break;
  }
  case 'chrome:': {
    break;
  }
  default: {
    // Override default web functions.
    const { windowSetup } = require('@electron/internal/renderer/window-setup');
    windowSetup(guestInstanceId, openerId, isHiddenPage, usesNativeWindowOpen);
  }
}

// Load webview tag implementation.
if (process.isMainFrame) {
  const { webViewInit } = require('@electron/internal/renderer/web-view/web-view-init');
  webViewInit(contextIsolation, webviewTag, guestInstanceId);
}

if (nodeIntegration) {
  // Export node bindings to global.
  const { makeRequireFunction } = __non_webpack_require__('internal/modules/cjs/helpers') // eslint-disable-line
  global.module = new Module('electron/js2c/renderer_init');
  global.require = makeRequireFunction(global.module);

  // Set the __filename to the path of html file if it is file: protocol.
  if (window.location.protocol === 'file:') {
    const location = window.location;
    let pathname = location.pathname;

    if (process.platform === 'win32') {
      if (pathname[0] === '/') pathname = pathname.substr(1);

      const isWindowsNetworkSharePath = location.hostname.length > 0 && process.resourcesPath.startsWith('\\');
      if (isWindowsNetworkSharePath) {
        pathname = `//${location.host}/${pathname}`;
      }
    }

    global.__filename = path.normalize(decodeURIComponent(pathname));
    global.__dirname = path.dirname(global.__filename);

    // Set module's filename so relative require can work as expected.
    global.module.filename = global.__filename;

    // Also search for module under the html file.
    global.module.paths = Module._nodeModulePaths(global.__dirname);
  } else {
    // For backwards compatibility we fake these two paths here
    global.__filename = path.join(process.resourcesPath, 'electron.asar', 'renderer', 'init.js');
    global.__dirname = path.join(process.resourcesPath, 'electron.asar', 'renderer');

    if (appPath) {
      // Search for module under the app directory
      global.module.paths = Module._nodeModulePaths(appPath);
    }
  }

  // Redirect window.onerror to uncaughtException.
  window.onerror = function (_message, _filename, _lineno, _colno, error) {
    if (global.process.listenerCount('uncaughtException') > 0) {
      // We do not want to add `uncaughtException` to our definitions
      // because we don't want anyone else (anywhere) to throw that kind
      // of error.
      global.process.emit('uncaughtException', error as any);
      return true;
    } else {
      return false;
    }
  };
} else {
  // Delete Node's symbols after the Environment has been loaded in a
  // non context-isolated environment
  if (!contextIsolation) {
    process.once('loaded', function () {
      delete (global as any).process;
      delete (global as any).Buffer;
      delete (global as any).setImmediate;
      delete (global as any).clearImmediate;
      delete (global as any).global;
      delete (global as any).root;
      delete (global as any).GLOBAL;
    });
  }
}

// Load the preload scripts.
for (const preloadScript of preloadScripts) {
  try {
    Module._load(preloadScript);
  } catch (error) {
    console.error(`Unable to load preload script: ${preloadScript}`);
    console.error(error);

    ipcRendererInternal.send(IPC_MESSAGES.BROWSER_PRELOAD_ERROR, preloadScript, error);
  }
}

// Warn about security issues
if (process.isMainFrame) {
  const { securityWarnings } = require('@electron/internal/renderer/security-warnings');
  securityWarnings(nodeIntegration);
}
