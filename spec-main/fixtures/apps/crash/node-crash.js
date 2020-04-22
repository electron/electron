process.crashReporter.start({
  productName: 'Zombies',
  companyName: 'Umbrella Corporation',
  crashesDirectory: process.argv[4],
  submitURL: process.argv[2],
  ignoreSystemCrashHandler: true,
  extra: {
    extra1: 'extra1',
    extra2: 'extra2',
    _version: process.argv[3]
  }
});

if (process.platform !== 'linux') {
  process.crashReporter.addExtraParameter('newExtra', 'newExtra');
  process.crashReporter.addExtraParameter('removeExtra', 'removeExtra');
  process.crashReporter.removeExtraParameter('removeExtra');
}

process.nextTick(() => process.crash());
