import { IPC_MESSAGES } from '@electron/internal/common/ipc-messages';
import type * as ipcRendererUtilsModule from '@electron/internal/renderer/ipc-renderer-internal-utils';
import { createPreloadProcessObject, executeSandboxedPreloadScripts } from '@electron/internal/sandboxed_renderer/preload';

import * as events from 'events';

declare const binding: {
  get: (name: string) => any;
  process: NodeJS.Process;
  createPreloadScript: (src: string) => Function
};

const { EventEmitter } = events;

process._linkedBinding = binding.get;

const v8Util = process._linkedBinding('electron_common_v8_util');
// Expose Buffer shim as a hidden value. This is used by C++ code to
// deserialize Buffer instances sent from browser process.
v8Util.setHiddenValue(global, 'Buffer', Buffer);
// The process object created by webpack is not an event emitter, fix it so
// the API is more compatible with non-sandboxed renderers.
for (const prop of Object.keys(EventEmitter.prototype) as (keyof typeof process)[]) {
  if (Object.hasOwn(process, prop)) {
    delete process[prop];
  }
}
Object.setPrototypeOf(process, EventEmitter.prototype);

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

Object.assign(process, binding.process);
Object.assign(process, processProps);

require('@electron/internal/renderer/ipc-native-setup');

executeSandboxedPreloadScripts({
  loadedModules: loadedModules,
  loadableModules: loadableModules,
  process: preloadProcess,
  createPreloadScript: binding.createPreloadScript,
  exposeGlobals: {
    Buffer: Buffer,
    global: global
  }
}, preloadScripts);
