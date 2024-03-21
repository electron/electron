import * as events from 'events';
import { setImmediate, clearImmediate } from 'timers';
import { IPC_MESSAGES } from '@electron/internal/common/ipc-messages';

import type * as ipcRendererUtilsModule from '@electron/internal/renderer/ipc-renderer-internal-utils';
import type * as ipcRendererInternalModule from '@electron/internal/renderer/ipc-renderer-internal';

declare const binding: {
  get: (name: string) => any;
  process: NodeJS.Process;
  createPreloadScript: (src: string) => Function;
};

const { ipcRendererInternal } = require('@electron/internal/renderer/ipc-renderer-internal') as typeof ipcRendererInternalModule;
const ipcRendererUtils = require('@electron/internal/renderer/ipc-renderer-internal-utils') as typeof ipcRendererUtilsModule;

const {
  preloadScripts,
  process: processProps
} = ipcRendererUtils.invokeSync<{
  preloadScripts: {
    preloadPath: string;
    preloadSrc: string | null;
    preloadError: null | Error;
  }[];
  process: NodeJS.Process;
}>(IPC_MESSAGES.BROWSER_SANDBOX_LOAD);

// Create a new EventEmitter instance and assign it to process to ensure compatibility with non-sandboxed renderers
process = new events.EventEmitter() as NodeJS.Process;
Object.assign(process, processProps);

process.getProcessMemoryInfo = () => {
  return ipcRendererInternal.invoke<Electron.ProcessMemoryInfo>(IPC_MESSAGES.BROWSER_GET_PROCESS_MEMORY_INFO);
};

// Execute preload scripts
for (const { preloadPath, preloadSrc, preloadError } of preloadScripts) {
  try {
    if (preloadSrc) {
      const runPreloadScript = () => {
        const preloadWrapperSrc = `(function(require, process, Buffer, global, setImmediate, clearImmediate, exports, module) {
          ${preloadSrc}
        })`;
        const preloadFn = binding.createPreloadScript(preloadWrapperSrc);
        const exports = {};
        preloadFn(preloadRequire, process, Buffer, global, setImmediate, clearImmediate, exports, { exports });
      };
      runPreloadScript();
    } else if (preloadError) {
      throw preloadError;
    }
  } catch (error) {
    console.error(`Unable to load preload script: ${preloadPath}`);
    console.error(error);
    ipcRendererInternal.send(IPC_MESSAGES.BROWSER_PRELOAD_ERROR, preloadPath, error);
  }
}

// Function to load required modules in preload script
function preloadRequire(module: string) {
  if (loadedModules.has(module)) {
    return loadedModules.get(module);
  }
  if (loadableModules.has(module)) {
    const loadedModule = loadableModules.get(module)!();
    loadedModules.set(module, loadedModule);
    return loadedModule;
  }
  throw new Error(`module not found: ${module}`);
}
