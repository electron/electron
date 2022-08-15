self.onconnect = function (event) {
  const port = event.ports[0];
  port.start();
  port.postMessage([typeof process, typeof setImmediate, typeof global, typeof Buffer].join(' '));
};
