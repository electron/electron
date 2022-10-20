const result = require('child_process').execSync('sudo --help');
process.parentPort.postMessage(result);
