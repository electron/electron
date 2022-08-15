self.onconnect = function (e) {
  const port = e.ports[0];
  port.postMessage('ready');
};
