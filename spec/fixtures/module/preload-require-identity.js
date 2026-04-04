/* eslint-disable no-self-compare, import/enforce-node-protocol-usage */
// Regression test fixture for the esbuild __toCommonJS identity break.
// Each pair must be the exact same object reference — see the
// "module identity" describe block in spec/modules-spec.ts. The comparisons
// are intentionally self-referential and intentionally use both the bare and
// `node:`-prefixed module names, which is why no-self-compare and
// import/enforce-node-protocol-usage are disabled for this file.
const { ipcRenderer } = require('electron');

ipcRenderer.send('require-identity', {
  electron: require('electron') === require('electron'),
  electronCommon: require('electron/common') === require('electron'),
  events: require('events') === require('node:events'),
  timers: require('timers') === require('node:timers'),
  url: require('url') === require('node:url'),
  ipcRenderer: require('electron').ipcRenderer === require('electron').ipcRenderer,
  contextBridge: require('electron').contextBridge === require('electron').contextBridge
});
