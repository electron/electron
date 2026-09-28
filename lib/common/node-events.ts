// The `events` module for bundles that run without a Node.js environment (the
// <webview> element bundles): the bundler resolves `import ... from 'events'` here
// for those targets. `EventEmitter` is implemented natively; see
// shell/common/gin_helper/node_event_emitter.cc.
const { EventEmitter } = process._linkedBinding('electron_common_events');

export { EventEmitter };
