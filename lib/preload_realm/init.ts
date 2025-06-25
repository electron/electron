import '@electron/internal/sandboxed_renderer/pre-init';
import { IPC_MESSAGES } from '@electron/internal/common/ipc-messages';
import type * as ipcRendererUtilsModule from '@electron/internal/renderer/ipc-renderer-internal-utils';
import { createPreloadProcessObject, executeSandboxedPreloadScripts } from '@electron/internal/sandboxed_renderer/preload';

import * as events from 'events';

declare const binding: {
  get: (name: string) => any;
  process: NodeJS.Process;
  createPreloadScript: (src: string) => Function
};

const ipcRendererUtils = require('@electron/internal/renderer/ipc-renderer-internal-utils') as typeof ipcRendererUtilsModule;

const {
  preloadScripts,
  process: processProps
} = ipcRendererUtils.invokeSync<{
  preloadScripts: ElectronInternal.PreloadScript[];
  process: NodeJS.Process;
}>(IPC_MESSAGES.BROWSER_SANDBOX_LOAD);

const electron = require('electron');

const loadedModules = new Map<string, any>([
  ['electron', electron],
  ['electron/common', electron],
  ['events', events],
  ['node:events', events]
]);

const loadableModules = new Map<string, Function>([
  ['url', () => require('url')],
  ['node:url', () => require('url')]
]);

const preloadProcess = createPreloadProcessObject();

Object.assign(preloadProcess, binding.process);
Object.assign(preloadProcess, processProps);

Object.assign(process, processProps);

require('@electron/internal/renderer/ipc-native-setup');

executeSandboxedPreloadScripts({
  loadedModules,
  loadableModules,
  process: preloadProcess,
  createPreloadScript: binding.createPreloadScript,
  exposeGlobals: {
    Buffer,
    // FIXME(samuelmaddock): workaround webpack bug replacing this with just
    // `__webpack_require__.g,` which causes script error
    global: globalThis
  }
}, preloadScripts);
