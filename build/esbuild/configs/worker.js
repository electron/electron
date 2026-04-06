module.exports = {
  target: 'worker',
  loadElectronFromAlternateTarget: 'renderer',
  alwaysHasNode: true,
  targetDeletesNodeGlobals: true,
  wrapInitWithTryCatch: true
};
