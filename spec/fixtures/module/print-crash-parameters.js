process.crashReporter.addExtraParameter('hello', 'world');
process.stdout.write(JSON.stringify(process.crashReporter.getParameters()) + '\n');
