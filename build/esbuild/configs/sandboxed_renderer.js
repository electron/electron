module.exports = {
  target: 'sandboxed_renderer',
  alwaysHasNode: false,
  wrapInitWithProfilingTimeout: true,
  wrapInitWithTryCatch: true
};
