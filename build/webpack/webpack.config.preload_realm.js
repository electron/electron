module.exports = require('./webpack.config.base')({
  target: 'preload_realm',
  alwaysHasNode: false,
  wrapInitWithProfilingTimeout: true,
  wrapInitWithTryCatch: true
});
