const Module = require('node:module');

process.send(Module._nodeModulePaths(process.resourcesPath + '/test.js'));
