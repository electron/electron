const native = require('../build/Release/print_handler.node');

// Arm before the dialog opens: the system print dialog runs a modal loop, so
// the watcher (an NSTimer in NSRunLoopCommonModes on macOS, a thread on
// Windows) has to already be running to confirm or cancel it.
function startWatching(action = 'cancel', timeoutMs = 5000) {
  native.startWatching(action, timeoutMs);
}

// Returns whether a dialog was dismissed since startWatching().
function stopWatching() {
  return native.stopWatching();
}

module.exports = { startWatching, stopWatching };
