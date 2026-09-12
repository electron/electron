const { parentPort, workerData } = require('node:worker_threads');

const { recordState } = require(workerData);
const v8Util = process._linkedBinding('electron_common_v8_util');
const object = {};
v8Util.setHiddenValue(object, 'callback-test', 42);
v8Util.requestGarbageCollectionForTesting();

const { snapshot } = recordState();
parentPort.postMessage({
  hasHolder: snapshot.some((node) => node.name === 'Electron / CallbackHolder' && node.type !== 'string'),
  value: v8Util.getHiddenValue(object, 'callback-test')
});
