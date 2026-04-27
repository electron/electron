const native = require('./build/Release/print_handler.node');

/**
 * Arm the print-dialog watcher. Call this BEFORE triggering
 * webContents.print() — the watcher uses an NSTimer in
 * NSRunLoopCommonModes so it fires during the modal run loop
 * that runModalWithPrintInfo: enters.
 *
 * @param {'cancel'|'print'} [action='cancel']
 * @param {number} [timeoutMs=5000]
 */
function startWatching (action = 'cancel', timeoutMs = 5000) {
  native.startWatching(action, timeoutMs);
}

/**
 * Stop the watcher and return whether a dialog was dismissed.
 * Call after the print callback has fired.
 *
 * @returns {boolean}
 */
function stopWatching () {
  return native.stopWatching();
}

module.exports = { startWatching, stopWatching };
