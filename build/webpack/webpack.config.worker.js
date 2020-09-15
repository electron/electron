module.exports = require('./webpack.config.base')({
  target: 'worker',
  loadElectronFromAlternateTarget: 'renderer',
  alwaysHasNode: true,
  targetDeletesNodeGlobals: true,
  wrapInitWithTryCatch: true
});
