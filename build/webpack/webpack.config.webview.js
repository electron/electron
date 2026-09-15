module.exports = require('./webpack.config.base')({
  target: 'webview',
  alwaysHasNode: false,
  loadElectronFromAlternateTarget: 'renderer',
  wrapInitWithTryCatch: true
});
