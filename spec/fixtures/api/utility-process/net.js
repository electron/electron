const { net } = require('electron');

function fail () {
  process.parentPort.postMessage(1);
}

process.parentPort.on('message', (e) => {
  const urlRequest = net.request(e.data);
  urlRequest.on('error', fail);
  urlRequest.on('abort', fail);
  urlRequest.on('response', (response) => {
    process.parentPort.postMessage(response.statusCode);
  });
  urlRequest.end();
});
