onconnect = function(event) {
  var port = event.ports[0];
  port.start();
  port.onmessage = function(event) {
    port.postMessage(event.data);
  }
}
