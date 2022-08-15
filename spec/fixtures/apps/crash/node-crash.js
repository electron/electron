if (process.platform === 'linux') {
  process.crashReporter.start({
    submitURL: process.argv[2],
    productName: 'Zombies',
    compress: false,
    globalExtra: {
      _version: process.argv[3]
    }
  });
}
process.nextTick(() => process.crash());
