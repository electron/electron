this.onconnect = function (event) {
  const port = event.ports[0];
  port.start();
  port.onmessage = function (event) {
    port.postMessage(event.data);
  };
};
