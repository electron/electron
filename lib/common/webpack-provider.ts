// This file provides the global, process and Buffer variables to internal
// Electron code once they have been deleted from the global scope.
//
// It does this through the ProvidePlugin in the webpack.config.base.js file
// Check out the Module.wrapper override in renderer/init.ts for more
// information on how this works and why we need it

// Rip global off of window (which is also global) so that webpack doesn't
// auto replace it with a looped reference to this file
const _global = typeof globalThis !== 'undefined' ? globalThis.global : (self || window).global;
const process = _global.process;
const Buffer = _global.Buffer;

export {
  _global,
  process,
  Buffer
};
