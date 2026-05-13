import '@electron/internal/sandboxed_renderer/pre-init';
import { IPC_MESSAGES } from '@electron/internal/common/ipc-messages';
import type * as ipcRendererUtilsModule from '@electron/internal/renderer/ipc-renderer-internal-utils';
import {
  createPreloadProcessObject,
  executeSandboxedPreloadScripts
} from '@electron/internal/sandboxed_renderer/preload';

import * as events from 'events';
import { setImmediate, clearImmediate } from 'timers';

declare const binding: {
  process: NodeJS.Process;
  createPreloadScript: (src: string) => Function;
  // Pushed by the browser via mojom.ElectronFrameStartup ahead of
  // CommitNavigation, or null if it hasn't arrived (e.g. the initial empty
  // document). When present, lets us skip the BROWSER_SANDBOX_LOAD sync IPC.
  startupData: {
    preloadScripts: ElectronInternal.PreloadScript[];
    process: NodeJS.Process;
  } | null;
};

const ipcRendererUtils =
  require('@electron/internal/renderer/ipc-renderer-internal-utils') as typeof ipcRendererUtilsModule;

// Prefer the data the browser pushed ahead of the navigation. The sync-IPC
// fallback exists only for cases the push doesn't cover yet (initial empty
// document, devtools, webview frames) — it parks the renderer main thread on a
// browser UI-thread roundtrip and amplifies under contention, so it's the slow
// path.
const { preloadScripts, process: processProps } =
  binding.startupData ??
  ipcRendererUtils.invokeSync<{
    preloadScripts: ElectronInternal.PreloadScript[];
    process: NodeJS.Process;
  }>(IPC_MESSAGES.BROWSER_SANDBOX_LOAD);

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
