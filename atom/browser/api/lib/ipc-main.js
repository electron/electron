const EventEmitter = require('events').EventEmitter;

module.exports = new EventEmitter;

// Every webContents would add a listenter to the
// WEB_FRAME_RESPONSE event, so ignore the listenters warning.
module.exports.setMaxListeners(0);
