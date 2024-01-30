const { parentPort } = require('node:worker_threads');
parentPort.postMessage(JSON.stringify(process));
