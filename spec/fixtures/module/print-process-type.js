// Prints process.type ('browser' in an app's main process, undefined when run
// as Node.js) and exits once it has been written.
process.stdout.write(`${process.type}\n`, () => process.exit(0));
