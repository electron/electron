module.exports = require('./webpack.config.base')({
  target: 'asar',
  alwaysHasNode: true,
  targetDeletesNodeGlobals: true
});
