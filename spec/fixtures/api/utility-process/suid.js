const result = require('node:child_process').execSync('sudo --help');

process.parentPort.postMessage(result);
