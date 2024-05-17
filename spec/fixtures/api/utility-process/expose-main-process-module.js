const { systemPreferences } = require('electron');

const status = systemPreferences.getMediaAccessStatus('screen');
process.parentPort.on('message', (e) => {
  console.log(e);
  process.parentPort.postMessage(status);
});
