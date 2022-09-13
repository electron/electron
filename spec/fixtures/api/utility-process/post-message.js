process.parentPort.on('message', (e) => {
  process.parentPort.postMessage(e.data);
});
