import { IPC_MESSAGES } from '@electron/internal/common/ipc-messages';
import type * as ipcRendererInternalModule from '@electron/internal/renderer/ipc-renderer-internal';

import { EventEmitter } from 'events';

// Delay loading for `process._linkedBinding` to be set.
const getIpcRendererLazy = () => require('@electron/internal/renderer/ipc-renderer-internal') as typeof ipcRendererInternalModule;

interface PreloadContext {
  loadedModules: Map<string, any>;
  loadableModules: Map<string, any>;

  /** Process object to pass into preloads. */
  process: NodeJS.Process;

  createPreloadScript: (src: string) => Function

  /** Globals to be exposed to preload context. */
  exposeGlobals: any;
}

export function createPreloadProcessObject (): NodeJS.Process {
  const preloadProcess: NodeJS.Process = new EventEmitter() as any;

  preloadProcess.getProcessMemoryInfo = () => {
    const { ipcRendererInternal } = getIpcRendererLazy();
    return ipcRendererInternal.invoke<Electron.ProcessMemoryInfo>(IPC_MESSAGES.BROWSER_GET_PROCESS_MEMORY_INFO);
  };

  Object.defineProperty(preloadProcess, 'noDeprecation', {
    get () {
      return process.noDeprecation;
    },
    set (value) {
      process.noDeprecation = value;
    }
  });

  const { hasSwitch } = process._linkedBinding('electron_common_command_line');

  // Similar to nodes --expose-internals flag, this exposes _linkedBinding so
  // that tests can call it to get access to some test only bindings
  if (hasSwitch('unsafely-expose-electron-internals-for-testing')) {
    preloadProcess._linkedBinding = process._linkedBinding;
  }

  return preloadProcess;
}

// This is the `require` function that will be visible to the preload script
function preloadRequire (context: PreloadContext, module: string) {
  if (context.loadedModules.has(module)) {
    return context.loadedModules.get(module);
  }
  if (context.loadableModules.has(module)) {
    const loadedModule = context.loadableModules.get(module)!();
    context.loadedModules.set(module, loadedModule);
    return loadedModule;
  }
  throw new Error(`module not found: ${module}`);
}

// Wrap the script into a function executed in global scope. It won't have
// access to the current scope, so we'll expose a few objects as arguments:
//
// - `require`: The `preloadRequire` function
// - `process`: The `preloadProcess` object
// - `Buffer`: Shim of `Buffer` implementation
// - `global`: The window object, which is aliased to `global` by webpack.
function runPreloadScript (context: PreloadContext, preloadSrc: string) {
  const globalVariables = [];
  const fnParameters = [];
  for (const [key, value] of Object.entries(context.exposeGlobals)) {
    globalVariables.push(key);
    fnParameters.push(value);
  }
  const preloadWrapperSrc = `(function(require, process, exports, module, ${globalVariables.join(', ')}) {
  ${preloadSrc}
  })`;

  // eval in window scope
  const preloadFn = context.createPreloadScript(preloadWrapperSrc);
  const exports = {};

  preloadFn(preloadRequire.bind(null, context), context.process, exports, { exports }, ...fnParameters);
}

/**
 * Execute preload scripts within a sandboxed process.
 */
export function executeSandboxedPreloadScripts (context: PreloadContext, preloadScripts: ElectronInternal.PreloadScript[]) {
  for (const { filePath, contents, error } of preloadScripts) {
    try {
      if (contents) {
        runPreloadScript(context, contents);
      } else if (error) {
        throw error;
      }
    } catch (error) {
      console.error(`Unable to load preload script: ${filePath}`);
      console.error(error);

      const { ipcRendererInternal } = getIpcRendererLazy();
      ipcRendererInternal.send(IPC_MESSAGES.BROWSER_PRELOAD_ERROR, filePath, error);
    }
  }
}
