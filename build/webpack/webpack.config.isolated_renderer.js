module.exports = require('./webpack.config.base')({
  target: 'isolated_renderer',
  alwaysHasNode: false,
  wrapInitWithTryCatch: true
});
