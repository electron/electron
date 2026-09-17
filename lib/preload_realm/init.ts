import '@electron/internal/sandboxed_renderer/pre-init';
import {
  createPreloadProcessObject,
  executeSandboxedPreloadScripts
} from '@electron/internal/sandboxed_renderer/preload';

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
  ['electron/common', electron]
]);

const preloadProcess = createPreloadProcessObject();

Object.assign(preloadProcess, binding.process);
Object.assign(preloadProcess, processProps);

Object.assign(process, processProps);

// Creates ipcRenderer up front so that messages from the browser have
// somewhere to go before anything imports it.
process._linkedBinding('electron_renderer_ipc');

executeSandboxedPreloadScripts(
  {
    loadedModules,
    process: preloadProcess,
    createPreloadScript: binding.createPreloadScript,
    exposeGlobals: {
      global: globalThis
    }
  },
  preloadScripts
);
