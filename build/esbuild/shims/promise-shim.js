// Captures the original `Promise` constructor so that userland mutations of
// `global.Promise.resolve` do not affect Electron's internal code. Mirrors
// webpack's ProvidePlugin reference to lib/common/webpack-globals-provider.

const _Promise = globalThis.Promise;

export { _Promise as Promise };
