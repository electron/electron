if (process.platform === 'linux') {
  process.crashReporter.start({
    submitURL: process.argv[2],
    // TODO: node should understand productName / version as the top level
    // options
    globalExtra: {
      _productName: 'Zombies',
      _version: process.argv[3]
    }
  });
}
process.nextTick(() => process.crash());
