import * as path from 'path';
import { pathToFileURL } from 'url';
import { IPC_MESSAGES } from '@electron/internal/common/ipc-messages';

import type { Electron } from 'electron';

import type * as ipcRendererInternalModule from '@electron/internal/renderer/ipc-renderer-internal';
import type * as ipcRendererUtilsModule from '@electron/internal/renderer/ipc-renderer-internal-utils';

const Module = require('module') as NodeJS.ModuleInternal;

// Suppress deprecated warnings for VM module in the renderer process
const originalModuleLoad = Module._load;
Module._load = function (request: string) {
  if (request === 'vm') {
    console.warn('The vm module of Node.js is deprecated in the renderer process and will be removed.');
  }
  return originalModuleLoad.apply(this, arguments as any);
};

// Restore process.argv
process.argv.splice(1, 1);

// Import common settings
require('@electron/internal/common/init');

const { ipcRendererInternal } = require('@electron/internal/renderer/ipc-renderer-internal') as typeof ipcRendererInternalModule;
const ipcRendererUtils = require('@electron/internal/renderer/ipc-renderer-internal-utils') as typeof ipcRendererUtilsModule;

// Get process memory info
process.getProcessMemoryInfo = () => {
  return ipcRendererInternal.invoke<Electron.ProcessMemoryInfo>(IPC_MESSAGES.BROWSER_GET_PROCESS_MEMORY_INFO);
};

// Process command line arguments
const { hasSwitch, getSwitchValue } = process._linkedBinding('electron_common_command_line');
const { mainFrame } = process._linkedBinding('electron_renderer_web_frame');

const nodeIntegration = mainFrame.getWebPreference('nodeIntegration');
const appPath = hasSwitch('app-path') ? getSwitchValue('app-path') : null;

// Common renderer initialization
require('@electron/internal/renderer/common-init');

if (nodeIntegration) {
  // Export node bindings to global
  const { makeRequireFunction } = __non_webpack_require__('internal/modules/helpers');
  global.module = new Module('electron/js2c/renderer_init');
  global.require = makeRequireFunction(global.module);

  // Set __filename and __dirname
  if (window.location.protocol === 'file:') {
    // Set __filename to the path of html file
    const { pathname, host } = window.location;
    let filename = path.normalize(decodeURIComponent(pathname));
    if (process.platform === 'win32' && host.length > 0 && process.resourcesPath.startsWith('\\')) {
      filename = `//${host}/${filename}`;
    }
    global.__filename = filename;
    global.__dirname = path.dirname(global.__filename);
    global.module.filename = global.__filename;
    global.module.paths = Module._nodeModulePaths(global.__dirname);
  } else {
    // Fallback paths for non-file protocol
    global.__filename = path.join(process.resourcesPath, 'electron.asar', 'renderer', 'init.js');
    global.__dirname = path.join(process.resourcesPath, 'electron.asar', 'renderer');
    if (appPath) {
      global.module.paths = Module._nodeModulePaths(appPath);
    }
  }

  // Redirect window.onerror to uncaughtException
  window.onerror = function (_message, _filename, _lineno, _colno, error) {
    if (global.process.listenerCount('uncaughtException') > 0) {
      global.process.emit('uncaughtException', error as any);
      return true;
    } else {
      return false;
    }
  };
} else if (!process.contextIsolated) {
  // Delete Node's symbols after Environment has been loaded in a non context-isolated environment
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

const { appCodeLoaded } = process;
delete process.appCodeLoaded;

const { preloadPaths } = ipcRendererUtils.invokeSync<{ preloadPaths: string[] }>(IPC_MESSAGES.BROWSER_NONSANDBOX_LOAD);
const [cjsPreloads, esmPreloads] = preloadPaths.reduce((acc, preload) => {
  const ext = path.extname(preload);
  acc[ext === '.mjs' ? 1 : 0].push(preload);
  return acc;
}, [[], []] as [string[], string[]]);

// Load the preload scripts
const loadPreloads = async (preloads: string[], isESM: boolean) => {
  const { loadESM } = __non_webpack_require__('internal/process/esm_loader');

  for (const preloadScript of preloads) {
    try {
      if (isESM) {
        await loadESM(async (esmLoader: any) => {
          await esmLoader.import(pathToFileURL(preloadScript).toString(), undefined, Object.create(null)).catch((err: Error) => {
            console.error(`Unable to load preload script: ${preloadScript}`);
            console.error(err);
            ipcRendererInternal.send(IPC_MESSAGES.BROWSER_PRELOAD_ERROR, preloadScript, err);
          });
        });
      } else {
        Module._load(preloadScript);
      }
    } catch (error) {
      console.error(`Unable to load preload script: ${preloadScript}`);
      console.error(error);
      ipcRendererInternal.send(IPC_MESSAGES.BROWSER_PRELOAD_ERROR, preloadScript, error);
    }
  }
};

Promise.all([
  loadPreloads(cjsPreloads, false),
  loadPreloads(esmPreloads, true)
]).finally(() => appCodeLoaded!());
