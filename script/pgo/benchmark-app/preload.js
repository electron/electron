// Preload for the IPC/contextBridge workload window.
//
// Exposes benchmark APIs across the context bridge so main-world JavaScript
// can exercise the bridge's marshaling paths (electron_api_context_bridge.cc).
// contextBridge is the hottest Electron-specific code in real applications
// and is invisible to both Chrome's PGO profile and browser-only benchmark
// training, so it must be exercised explicitly.
const { contextBridge, ipcRenderer } = require('electron');

contextBridge.exposeInMainWorld('pgoBridge', {
  // Pure bridge marshaling: value crosses the bridge in both directions with
  // no IPC behind it. Exercises proxying of primitives, objects, arrays, and
  // typed arrays.
  echo: (value) => value,

  // Object transformation across the bridge (allocates in the isolated world).
  transform: (obj) => ({ ...obj, bridged: true, ts: Date.now() }),

  // Function-argument marshaling: the callback is proxied back into the main
  // world and invoked from the isolated world.
  withCallback: (value, callback) => callback(value),

  // Bridge + IPC round trip at arbitrary payload sizes.
  invoke: (channel, payload) => ipcRenderer.invoke(channel, payload)
});
