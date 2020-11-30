module.exports = require('./webpack.config.base')({
  target: 'sandboxed_renderer',
  alwaysHasNode: false,
  wrapInitWithProfilingTimeout: true,
  wrapInitWithTryCatch: true
});
