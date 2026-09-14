// Regression fixture: constructing a WASI instance registers fast API
// bindings for every wasiImport function. Optimising a call to one of them
// must not read stale memory left behind by the binding setup.
const v8 = require('node:v8');

v8.setFlagsFromString('--allow-natives-syntax');

const { WASI } = require('node:wasi');

const wasi = new WASI({ version: 'preview1', returnOnExit: true });
const memory = new WebAssembly.Memory({ initial: 1 });
wasi.finalizeBindings({ exports: { memory } }, { memory });

// eslint-disable-next-line no-new-func
const run = new Function(
  'wasiImport',
  `
  function hot () { return wasiImport.fd_sync(3); }
  %PrepareFunctionForOptimization(hot);
  hot();
  hot();
  %OptimizeFunctionOnNextCall(hot);
  return hot();
`
);

const result = run(wasi.wasiImport);
process.stdout.write(typeof result === 'number' ? 'ok' : 'unexpected');
process.exit(0);
