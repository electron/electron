// Also run directly with ELECTRON_RUN_AS_NODE.
const report = () => ({
  env: Object.keys(process.env)
    .filter((name) => name.startsWith('NODE_'))
    .sort(),
  compileCache: require('node:module').getCompileCacheDir() !== undefined,
  proxy: require('node:http').globalAgent.options.proxyEnv !== undefined
});

if (process.env.ELECTRON_RUN_AS_NODE) {
  console.log(JSON.stringify(report()));
} else if (process.parentPort) {
  process.parentPort.postMessage(report());
} else {
  module.exports = report;
}
