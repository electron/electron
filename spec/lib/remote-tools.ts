// Anything an itremote()/remotely() closure references via `import` must come
// from here. vite's SSR transform rewrites the spec file's import of this
// module to `__vite_ssr_import_N__`, so `path.join(...)` in a closure becomes
// `__vite_ssr_import_N__.path.join(...)`. runRemote()/remotely() replace every
// `__vite_ssr_import_\d+__` with `__rt`, a renderer-side object whose keys
// mirror these export names exactly — so `__rt.path.join(...)` resolves
// correctly with no property-name guessing.

export * as path from 'node:path';
export * as fs from 'node:fs';
export * as url from 'node:url';
export * as util from 'node:util';
export * as os from 'node:os';
export * as cp from 'node:child_process';
export * as http from 'node:http';
export { once } from 'node:events';
export { setTimeout } from 'node:timers/promises';
export { expect } from 'chai';
export { BrowserWindow, nativeImage, net, session, webContents } from 'electron/main';
// Renderer-only; undefined when this module is loaded in the main process,
// but typed correctly for closures stringified into preload scripts.
export { contextBridge, ipcRenderer, webFrame } from 'electron/renderer';
export { defer } from './defer-helpers';
export {
  collectStreamBody,
  collectStreamBodyBuffer,
  getResponse,
  kOneKiloByte,
  kOneMegaByte,
  randomBuffer,
  randomString,
  respondNTimes,
  respondOnce
} from './net-helpers';

// Renderer-side mirror of the exports above. Keep the keys in sync.
// Context-specific targets (e.g. the api-net utility-process fixture) can
// declare their own __rt with additional keys for helpers that aren't
// require()-able from a renderer.
export const REMOTE_TOOLS_SHIM = `{
  path: require('node:path'),
  fs: require('node:fs'),
  url: require('node:url'),
  util: require('node:util'),
  os: require('node:os'),
  cp: require('node:child_process'),
  http: require('node:http'),
  once: require('node:events').once,
  setTimeout: require('node:timers/promises').setTimeout,
  expect: require('chai').expect,
  BrowserWindow: require('electron').BrowserWindow,
  nativeImage: require('electron').nativeImage,
  net: require('electron').net,
  session: require('electron').session,
  webContents: require('electron').webContents,
}`;

const SSR_IMPORT_RE = /__vite_ssr_import_\d+__/g;

export function rewriteForRemoteEval(fn: Function): string {
  return String(fn).replace(SSR_IMPORT_RE, '__rt');
}
