process.parentPort.on('message', (e) => {
  e.ports[0].on('message', (ev) => {
    process.parentPort.postMessage(ev.data);
  });
  e.ports[0].start();
});
