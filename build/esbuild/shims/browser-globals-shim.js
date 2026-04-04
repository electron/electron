// Injected into browser-platform bundles (sandboxed_renderer, isolated_renderer,
// preload_realm) where Node globals are not implicitly available. Supplies
// `Buffer`, `process`, and `global` — replacing webpack's ProvidePlugin
// polyfill injection plus webpack 5's built-in `global -> globalThis` rewrite
// that `target: 'web'` performed automatically.
//
// The `buffer` and `process/browser` imports below intentionally use the
// npm polyfill packages, not Node's built-in `node:buffer` / `node:process`
// modules, because these shims ship into browser-platform bundles that do
// not have Node globals available at runtime.

/* eslint-disable import/order, import/enforce-node-protocol-usage */
import { Buffer as _Buffer } from 'buffer';
import _process from 'process/browser';

const _global = globalThis;

export { _Buffer as Buffer, _process as process, _global as global };
