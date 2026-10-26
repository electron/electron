declare const binding: {
  get: NodeJS.Process['_linkedBinding'];
  contextIsolated: boolean;
};

// The `process` free variable the bundler injects into the modules of the bundles
// that run without Node.js (lib/webview and lib/isolated_renderer): just what
// the shared renderer modules they include use.
const process = {
  _linkedBinding: binding.get,
  contextIsolated: binding.contextIsolated,
  sandboxed: true,
  type: 'renderer'
};

export default process;
