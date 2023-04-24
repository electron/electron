const vm = require('node:vm');

const contextObject = { result: 0 };
vm.createContext(contextObject);
vm.runInContext('eval(\'result = 42\')', contextObject);
process.parentPort.postMessage(contextObject.result);
