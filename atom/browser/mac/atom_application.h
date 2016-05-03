// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#import "base/mac/scoped_sending_event.h"

@interface AtomApplication : NSApplication<CrAppProtocol,
                                           CrAppControlProtocol> {
 @private
  BOOL handlingSendEvent_;
  NSUserActivity* currentActivity_;
}

+ (AtomApplication*)sharedApplication;

// CrAppProtocol:
- (BOOL)isHandlingSendEvent;

// CrAppControlProtocol:
- (void)setHandlingSendEvent:(BOOL)handlingSendEvent;

- (NSUserActivity*)getCurrentActivity;

- (void)setCurrentActivity:(NSUserActivity*)userActivity;

@end
