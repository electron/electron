process.parentPort.on('message', () => {});
process.parentPort.postMessage('ready');
setInterval(() => {}, 1000);
