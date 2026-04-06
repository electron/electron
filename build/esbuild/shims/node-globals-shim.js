// Injected into renderer/worker bundles to replace webpack's ProvidePlugin
// that captured `Buffer`, `global`, and `process` before user code could
// delete them from the global scope. The Module.wrapper override in
// lib/renderer/init.ts re-injects these into user preload scripts later.

// Rip globals off of globalThis/self/window so they are captured in this
// module's closure and retained even if the caller later deletes them.
const _global = typeof globalThis !== 'undefined'
  ? globalThis.global
  : (self || window).global;
const _process = _global.process;
const _Buffer = _global.Buffer;

export { _global as global, _process as process, _Buffer as Buffer };
