// Once given a count and a MessagePortMain, posts that many messages on
// parentPort, then posts on the port and exits when the parent replies there.
// process.exit() does not wait for pending postMessage() writes, so exiting
// straight away could lose messages. parentPort and the port share the same
// ordered Mojo channel to the parent, so by the time the parent sees the
// message on the port, every parentPort message posted before it has arrived.
process.parentPort.once('message', ({ data: count, ports: [port] }) => {
  for (let index = 0; index < count; index++) {
    process.parentPort.postMessage(index);
  }
  port.once('message', () => process.exit(0));
  port.start();
  port.postMessage('posted');
});
