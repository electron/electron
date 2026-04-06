module.exports = {
  target: 'preload_realm',
  alwaysHasNode: false,
  wrapInitWithProfilingTimeout: true,
  wrapInitWithTryCatch: true
};
