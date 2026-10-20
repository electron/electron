// Exits once the parent has acknowledged the message: process.exit() does not
// wait for the asynchronous postMessage() write to reach the parent.
process.parentPort.once('message', () => process.exit(0));
process.parentPort.postMessage(process.env.FROM);
