process.parentPort.on('message', (e) => {
  e.ports[0].on('message', (ev) => {
    e.ports[1].postMessage(ev.data);
  });
  e.ports[0].start();
});
