process.preloadRun = true;

exports.onBuiltinModulesPatched = () => {
  require('node:module')._nodeModulePaths = () => { return ['patched']; };
};
