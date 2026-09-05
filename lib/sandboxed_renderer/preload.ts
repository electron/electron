import { IPC_MESSAGES } from '@electron/internal/common/ipc-messages';
import { ipcRendererInternal } from '@electron/internal/renderer/ipc-renderer-internal';

import { EventEmitter } from 'events';

interface PreloadContext {
  loadedModules: Map<string, any>;
  loadableModules: Map<string, any>;

  /** Process object to pass into preloads. */
  process: NodeJS.Process;

  /**
   * Compiles a preload script's body and returns the resulting function. The
   * preload contents and V8 code cache are looked up by id from the mojo
   * startup data on the C++ side — they never become V8 strings until the
   * single copy for the compile, and the cache bytes never enter JS at all.
   * `paramNames` is the function's parameter list (CompileFunction(), not a
   * string-templated wrapper, so preload stack traces have correct line
   * numbers). Returns null if the script id is unknown.
   */
  createPreloadScript: (scriptId: string, paramNames: string[]) => Function | null;

  /** Globals to be exposed to preload context. */
  exposeGlobals: any;
}

export function createPreloadProcessObject(): NodeJS.Process {
  const preloadProcess: NodeJS.Process = new EventEmitter() as any;

  preloadProcess.getProcessMemoryInfo = () => {
    return ipcRendererInternal.invoke<Electron.ProcessMemoryInfo>(IPC_MESSAGES.BROWSER_GET_PROCESS_MEMORY_INFO);
  };

  Object.defineProperty(preloadProcess, 'noDeprecation', {
    get() {
      return process.noDeprecation;
    },
    set(value) {
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
function preloadRequire(context: PreloadContext, module: string) {
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

// Compile and run a preload script as a function with these parameters in
// scope, plus whatever's in `context.exposeGlobals`:
//
// - `require`: The `preloadRequire` function
// - `process`: The `preloadProcess` object
// - `Buffer`: Shim of `Buffer` implementation
// - `global`: The window object, which is aliased to `global` by webpack.
function runPreloadScript(context: PreloadContext, script: ElectronInternal.PreloadScript) {
  const globalVariables = [];
  const fnParameters = [];
  for (const [key, value] of Object.entries(context.exposeGlobals)) {
    globalVariables.push(key);
    fnParameters.push(value);
  }
  // The body and code cache are looked up from the mojo-cached startup data on
  // the C++ side keyed by script.id — neither crosses the V8 boundary. The
  // (paramNames, contents) pair is a deterministic function of (preload file,
  // Electron version), which is what makes the persisted code cache valid
  // across navigations and launches.
  const paramNames = ['require', 'process', 'exports', 'module', ...globalVariables];
  const preloadFn = context.createPreloadScript(script.id, paramNames);
  if (!preloadFn) return;
  const exports = {};

  preloadFn(preloadRequire.bind(null, context), context.process, exports, { exports }, ...fnParameters);
}

/**
 * Execute preload scripts within a sandboxed process.
 */
export function executeSandboxedPreloadScripts(
  context: PreloadContext,
  preloadScripts: ElectronInternal.PreloadScript[]
) {
  for (const script of preloadScripts) {
    const { filePath, hasContents, error } = script;
    try {
      if (hasContents) {
        runPreloadScript(context, script);
      } else if (error) {
        // eslint-disable-next-line no-throw-literal
        throw error;
      }
    } catch (error) {
      console.error(`Unable to load preload script: ${filePath}`);
      console.error(error);
      ipcRendererInternal.send(IPC_MESSAGES.BROWSER_PRELOAD_ERROR, filePath, error);
    }
  }
}
