import '@electron/internal/sandboxed_renderer/pre-init';
import {
  createPreloadProcessObject,
  executeSandboxedPreloadScripts
} from '@electron/internal/sandboxed_renderer/preload';

import * as events from 'events';
import { setImmediate, clearImmediate } from 'timers';

declare const binding: {
  process: NodeJS.Process;
  createPreloadScript: (scriptId: string, paramNames: string[]) => Function | null;
  // Pushed by the browser via mojom.ElectronFrameStartup at frame creation
  // and again ahead of every CommitNavigation, so it should always be present
  // by the time this bundle runs. The null fallback below is defensive: an
  // unexpected edge case is degraded to a preload-less init with a warning
  // instead of a TypeError that would abort the whole bundle.
  startupData: {
    preloadScripts: ElectronInternal.PreloadScript[];
    process: NodeJS.Process;
  } | null;
};

if (!binding.startupData) {
  console.warn(
    `Electron: no startup data for document "${globalThis.location?.href}"; preload scripts will not run in this context.`
  );
}

const { preloadScripts, process: processProps } = binding.startupData ?? {
  preloadScripts: [],
  process: {} as NodeJS.Process
};

const electron = require('electron');

const loadedModules = new Map<string, any>([
  ['electron', electron],
  ['electron/common', electron],
  ['electron/renderer', electron],
  ['events', events],
  ['node:events', events]
]);

const loadableModules = new Map<string, Function>([
  ['timers', () => require('timers')],
  ['node:timers', () => require('timers')],
  ['url', () => require('url')],
  ['node:url', () => require('url')]
]);

const preloadProcess = createPreloadProcessObject();

// InvokeEmitProcessEvent in ElectronSandboxedRendererClient will look for this
const v8Util = process._linkedBinding('electron_common_v8_util');
v8Util.setHiddenValue(global, 'emit-process-event', (event: string) => {
  (process as events.EventEmitter).emit(event);
  (preloadProcess as events.EventEmitter).emit(event);
});

Object.assign(preloadProcess, binding.process);
Object.assign(preloadProcess, processProps);

// Test-only (DCHECK builds): expose the js2c code-cache status on the
// sandboxed preload's process shim for the code-cache spec.
if ((v8Util as any).getJs2cCodeCacheStatus) {
  (preloadProcess as any).getJs2cCodeCacheStatus = () => (v8Util as any).getJs2cCodeCacheStatus();
}

Object.assign(process, processProps);

// Common renderer initialization
require('@electron/internal/renderer/common-init');

executeSandboxedPreloadScripts(
  {
    loadedModules,
    loadableModules,
    process: preloadProcess,
    createPreloadScript: binding.createPreloadScript,
    exposeGlobals: {
      Buffer,
      // FIXME(samuelmaddock): workaround webpack bug replacing this with just
      // `__webpack_require__.g,` which causes script error
      global: globalThis,
      setImmediate,
      clearImmediate
    }
  },
  preloadScripts
);
