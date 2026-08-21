#ifndef SRC_PRINT_HANDLER_H_
#define SRC_PRINT_HANDLER_H_

namespace print_handler {

// Starts an NSTimer (in NSRunLoopCommonModes) that polls for a modal print
// dialog. When one appears, it dismisses it via [NSApp stopModalWithCode:].
// The timer fires during the modal run loop, which is required because
// runModalWithPrintInfo: blocks the main thread.
//
// |should_print| - true sends NSModalResponseOK, false sends
//                  NSModalResponseCancel.
// |timeout_ms|   - give up after this many milliseconds.
void StartWatching(bool should_print, int timeout_ms);

// Stops the watcher timer and returns whether a modal dialog was dismissed.
bool StopWatching();

}  // namespace print_handler

#endif  // SRC_PRINT_HANDLER_H_
