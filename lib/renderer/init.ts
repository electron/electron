import { IPC_MESSAGES } from '@electron/internal/common/ipc-messages';
import type * as ipcRendererInternalModule from '@electron/internal/renderer/ipc-renderer-internal';
import type * as ipcRendererUtilsModule from '@electron/internal/renderer/ipc-renderer-internal-utils';

import * as path from 'path';
import { pathToFileURL } from 'url';

const Module = require('module') as NodeJS.ModuleInternal;

// VM module is unsupported in renderer due to Blink incompatibilities
const originalModuleLoad = Module._load;
Module._load = function (request: string) {
  if (request === 'vm') {
    console.warn('The vm module of Node.js is unsupported in Electron\'s renderer process due to incompatibilities with the Blink rendering engine. Crashes are likely and avoiding the module is highly recommended. This module may be removed in a future release.');
  }
  return originalModuleLoad.apply(this, arguments as any);
};

// Preserve globals in preload scripts (update webpack ProvidePlugin if modified)
Module.wrapper = [
  '(function (exports, require, module, __filename, __dirname, process, global, Buffer) { ' +
  'return function (exports, require, module, __filename, __dirname) { ',
  '\n}.call(this, exports, require, module, __filename, __dirname); });'
];

process.argv.splice(1, 1);

require('@electron/internal/common/init');

const { ipcRendererInternal } = require('@electron/internal/renderer/ipc-renderer-internal') as typeof ipcRendererInternalModule;
const ipcRendererUtils = require('@electron/internal/renderer/ipc-renderer-internal-utils') as typeof ipcRendererUtilsModule;

process.getProcessMemoryInfo = () => {
  return ipcRendererInternal.invoke<Electron.ProcessMemoryInfo>(IPC_MESSAGES.BROWSER_GET_PROCESS_MEMORY_INFO);
};

const { hasSwitch, getSwitchValue } = process._linkedBinding('electron_common_command_line');
const { mainFrame } = process._linkedBinding('electron_renderer_web_frame');

const nodeIntegration = mainFrame.getWebPreference('nodeIntegration');
const appPath = hasSwitch('app-path') ? getSwitchValue('app-path') : null;

require('@electron/internal/renderer/common-init');

if (nodeIntegration) {
  const { makeRequireFunction } = __non_webpack_require__('internal/modules/helpers') as typeof import('@node/lib/internal/modules/helpers');
  global.module = new Module('electron/js2c/renderer_init');
  global.require = makeRequireFunction(global.module) as NodeRequire;

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

    global.module.filename = global.__filename;
    global.module.paths = Module._nodeModulePaths(global.__dirname);
  } else {
    global.__filename = path.join(process.resourcesPath, 'electron.asar', 'renderer', 'init.js');
    global.__dirname = path.join(process.resourcesPath, 'electron.asar', 'renderer');

    if (appPath) {
      global.module.paths = Module._nodeModulePaths(appPath);
    }
  }

  window.onerror = function (_message, _filename, _lineno, _colno, error) {
    if (global.process.listenerCount('uncaughtException') > 0) {
      global.process.emit('uncaughtException', error as any);
      return true;
    } else {
      return false;
    }
  };
} else {
  // Clean up Node symbols in non-context-isolated environment
  if (!process.contextIsolated) {
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

const { appCodeLoaded } = process;
delete process.appCodeLoaded;

const { preloadPaths } = ipcRendererUtils.invokeSync<{ preloadPaths: string[] }>(IPC_MESSAGES.BROWSER_NONSANDBOX_LOAD);
const cjsPreloads = preloadPaths.filter(p => path.extname(p) !== '.mjs');
const esmPreloads = preloadPaths.filter(p => path.extname(p) === '.mjs');
if (cjsPreloads.length) {
  for (const preloadScript of cjsPreloads) {
    try {
      Module._load(preloadScript);
    } catch (error) {
      console.error(`Unable to load preload script: ${preloadScript}`);
      console.error(error);

      ipcRendererInternal.send(IPC_MESSAGES.BROWSER_PRELOAD_ERROR, preloadScript, error);
    }
  }
}
if (esmPreloads.length) {
  const { runEntryPointWithESMLoader } = __non_webpack_require__('internal/modules/run_main') as typeof import('@node/lib/internal/modules/run_main');

  runEntryPointWithESMLoader(async (cascadedLoader: any) => {
    for (const preloadScript of esmPreloads) {
      await cascadedLoader.import(pathToFileURL(preloadScript).toString(), undefined, Object.create(null)).catch((err: Error) => {
        console.error(`Unable to load preload script: ${preloadScript}`);
        console.error(err);

        ipcRendererInternal.send(IPC_MESSAGES.BROWSER_PRELOAD_ERROR, preloadScript, err);
      });
    }
  }).finally(() => appCodeLoaded!());
} else {
  appCodeLoaded!();
}
