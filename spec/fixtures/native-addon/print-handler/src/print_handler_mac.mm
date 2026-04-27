#include "print_handler.h"

#import <Cocoa/Cocoa.h>

// Poller that runs during modal run loops (NSRunLoopCommonModes) to detect
// and dismiss the system print dialog shown by [NSPrintPanel
// runModalWithPrintInfo:].
@interface PrintDialogPoller : NSObject {
  NSTimer* _timer;
  BOOL _shouldPrint;
  int _remainingTicks;
  BOOL _dismissed;
}
@property(nonatomic, readonly) BOOL dismissed;
- (instancetype)initWithShouldPrint:(BOOL)shouldPrint timeoutMs:(int)timeoutMs;
- (void)start;
- (void)stop;
@end

@implementation PrintDialogPoller

@synthesize dismissed = _dismissed;

- (instancetype)initWithShouldPrint:(BOOL)shouldPrint timeoutMs:(int)timeoutMs {
  if ((self = [super init])) {
    _shouldPrint = shouldPrint;
    _remainingTicks = timeoutMs / 100;
    _dismissed = NO;
  }
  return self;
}

- (void)start {
  // Use timerWithTimeInterval + addTimer:forMode: so the timer fires during
  // modal event processing (NSPrintPanel's runModalWithPrintInfo: enters a
  // nested run loop in NSModalPanelRunLoopMode, which is part of
  // NSRunLoopCommonModes).
  _timer = [NSTimer timerWithTimeInterval:0.1
                                   target:self
                                 selector:@selector(tick:)
                                 userInfo:nil
                                  repeats:YES];
  [[NSRunLoop mainRunLoop] addTimer:_timer forMode:NSRunLoopCommonModes];
}

- (void)stop {
  [_timer invalidate];
  _timer = nil;
}

- (void)tick:(NSTimer*)timer {
  --_remainingTicks;

  if ([NSApp modalWindow] != nil) {
    NSModalResponse code =
        _shouldPrint ? NSModalResponseOK : NSModalResponseCancel;
    [NSApp stopModalWithCode:code];
    _dismissed = YES;
    [self stop];
    return;
  }

  if (_remainingTicks <= 0) {
    [self stop];
  }
}

@end

// ---------- C++ API ----------

static PrintDialogPoller* g_poller = nil;

namespace print_handler {

void StartWatching(bool should_print, int timeout_ms) {
  // Stop any previously running poller.
  if (g_poller) {
    [g_poller stop];
    g_poller = nil;
  }

  g_poller = [[PrintDialogPoller alloc] initWithShouldPrint:should_print
                                                  timeoutMs:timeout_ms];
  [g_poller start];
}

bool StopWatching() {
  if (!g_poller)
    return false;
  bool result = [g_poller dismissed];
  [g_poller stop];
  g_poller = nil;
  return result;
}

}  // namespace print_handler
