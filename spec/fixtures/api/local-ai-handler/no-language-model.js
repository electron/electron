process.parentPort.on('message', () => {
  process.parentPort.postMessage('ack');
});
