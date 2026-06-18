import '@electron/internal/sandboxed_renderer/pre-init';
import {
  createPreloadProcessObject,
  executeSandboxedPreloadScripts
} from '@electron/internal/sandboxed_renderer/preload';

import * as events from 'events';

declare const binding: {
  get: (name: string) => any;
  process: NodeJS.Process;
  createPreloadScript: (scriptId: string, paramNames: string[]) => Function | null;
  // Delivered by the browser via the service worker's EmbeddedWorkerStartParams
  // (ContentBrowserClient::GetServiceWorkerStartupData), marshalled onto the
  // worker thread with the rest of the start params — always present when this
  // bundle runs.
  startupData: {
    preloadScripts: ElectronInternal.PreloadScript[];
    process: NodeJS.Process;
  };
};

const { preloadScripts, process: processProps } = binding.startupData;

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
      global: globalThis
    }
  },
  preloadScripts
);
