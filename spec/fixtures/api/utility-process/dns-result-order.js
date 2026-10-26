const dns = require('node:dns');

process.parentPort.on('message', () => {
  process.parentPort.postMessage(dns.getDefaultResultOrder());
});
