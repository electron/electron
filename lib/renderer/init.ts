import { EventEmitter } from 'events';
import * as path from 'path';

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
const v8Util = process.electronBinding('v8_util');

const ipcEmitter = new EventEmitter();
const ipcInternalEmitter = new EventEmitter();
v8Util.setHiddenValue(global, 'ipc', ipcEmitter);
v8Util.setHiddenValue(global, 'ipc-internal', ipcInternalEmitter);

v8Util.setHiddenValue(global, 'ipcNative', {
  onMessage (internal: boolean, channel: string, ports: any[], args: any[], senderId: number) {
    const sender = internal ? ipcInternalEmitter : ipcEmitter;
    sender.emit(channel, { sender, senderId, ports }, ...args);
  }
});

// Use electron module after everything is ready.
const { ipcRendererInternal } = require('@electron/internal/renderer/ipc-renderer-internal');
const { webFrameInit } = require('@electron/internal/renderer/web-frame-init');
webFrameInit();

// Process command line arguments.
const { hasSwitch, getSwitchValue } = process.electronBinding('command_line');

const parseOption = function<T> (
  name: string, defaultValue: T, converter?: (value: string) => T
) {
  return hasSwitch(name)
    ? (
      converter
        ? converter(getSwitchValue(name))
        : getSwitchValue(name)
    )
    : defaultValue;
};

const contextIsolation = hasSwitch('context-isolation');
const nodeIntegration = hasSwitch('node-integration');
const webviewTag = hasSwitch('webview-tag');
const isHiddenPage = hasSwitch('hidden-page');
const usesNativeWindowOpen = hasSwitch('native-window-open');
const rendererProcessReuseEnabled = hasSwitch('disable-electron-site-instance-overrides');

const preloadScript = parseOption('preload', null);
const preloadScripts = parseOption('preload-scripts', [], value => value.split(path.delimiter)) as string[];
const appPath = parseOption('app-path', null);
const guestInstanceId = parseOption('guest-instance-id', null, value => parseInt(value));
const openerId = parseOption('opener-id', null, value => parseInt(value));

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
  case 'chrome:':
    break;
  default: {
    // Override default web functions.
    const { windowSetup } = require('@electron/internal/renderer/window-setup');
    windowSetup(guestInstanceId, openerId, isHiddenPage, usesNativeWindowOpen, rendererProcessReuseEnabled);
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
      global.process.emit('uncaughtException' as any, error as any);
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
      delete global.process;
      delete global.Buffer;
      delete global.setImmediate;
      delete global.clearImmediate;
      delete global.global;
      delete global.root;
      delete global.GLOBAL;
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

    ipcRendererInternal.send('ELECTRON_BROWSER_PRELOAD_ERROR', preloadScript, error);
  }
}

// Warn about security issues
if (process.isMainFrame) {
  const { securityWarnings } = require('@electron/internal/renderer/security-warnings');
  securityWarnings(nodeIntegration);
}
