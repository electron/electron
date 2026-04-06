module.exports = {
  target: 'renderer',
  alwaysHasNode: true,
  targetDeletesNodeGlobals: true,
  wrapInitWithProfilingTimeout: true,
  wrapInitWithTryCatch: true
};
