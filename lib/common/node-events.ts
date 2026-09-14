// The `events` module for bundles that run without a Node.js environment
// (sandboxed renderers and service worker preload realms): webpack resolves
// `import ... from 'events'` here for those targets. `EventEmitter` is
// implemented natively; see shell/common/gin_helper/node_event_emitter.cc.

// These bundles are invoked with the sandboxed renderer's `binding` object in
// scope. It is used directly because this module is evaluated before
// pre-init.ts has installed `process._linkedBinding`.
declare const binding: { get: NodeJS.Process['_linkedBinding'] };

const { EventEmitter } = binding.get('electron_common_events');

export { EventEmitter };
