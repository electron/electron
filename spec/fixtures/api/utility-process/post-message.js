process.parentPort.on('message', (e) => {
  e.ports[0].postMessage(e.data);
});
