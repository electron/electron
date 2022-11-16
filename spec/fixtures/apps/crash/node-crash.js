console.log('Calling process.node-crash...', process.pid);
process.nextTick(() => process.crash());
