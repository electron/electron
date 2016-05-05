// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#import "base/mac/scoped_sending_event.h"
#import "base/mac/scoped_nsobject.h"

@interface AtomApplication : NSApplication<CrAppProtocol,
                                           CrAppControlProtocol> {
 @private
  BOOL handlingSendEvent_;
  base::scoped_nsobject<NSUserActivity> currentActivity_;
}

+ (AtomApplication*)sharedApplication;

// CrAppProtocol:
- (BOOL)isHandlingSendEvent;

// CrAppControlProtocol:
- (void)setHandlingSendEvent:(BOOL)handlingSendEvent;

- (NSUserActivity*)getCurrentActivity;
- (void)setCurrentActivity:(NSString*)type withUserInfo:(NSDictionary*)userInfo;

@end
