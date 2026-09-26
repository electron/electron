// vitest only ships an ES module entry point and refuses `require('vitest')`.
// Code that cannot `import` it (a renderer's main world under `itremote`, the
// fixture apps and the utility-process harness, all CommonJS) loads the module
// file directly instead; Node.js can require() an ES module synchronously.
const path = require('node:path');

module.exports = require(path.resolve(require.resolve('vitest/package.json'), '..', 'dist', 'index.js'));
