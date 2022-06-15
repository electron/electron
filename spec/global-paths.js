const Module = require('module');

module.exports = (paths) => {
  const nodeModulePaths = Module._nodeModulePaths;
  Module.ignoreGlobalPathsHack = false;
  Module._nodeModulePaths = (from) => {
    if (Module.ignoreGlobalPathsHack) {
      return nodeModulePaths(from);
    } else {
      return nodeModulePaths(from).concat(paths);
    }
  };
};
