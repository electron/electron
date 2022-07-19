import { MessagePortMain } from '@electron/internal/browser/message-port-main';
const Module = require('module');

// We modified the original process.argv to let node.js load the init.js,
// we need to restore it here.
process.argv.splice(1, 1, process._serviceStartupScript);

// Clear search paths.
require('../common/reset-search-paths');

// Import common settings.
require('@electron/internal/common/init');

process.on('-ipc-ports' as any, function (event: any, channel: string, ports: any[]) {
  event.ports = ports.map(p => new MessagePortMain(p));
  process.emit(channel, event);
});

// Finally load entry script.
process._firstFileName = Module._resolveFilename(process._serviceStartupScript, null, false);
Module._load(process._serviceStartupScript, Module, true);
