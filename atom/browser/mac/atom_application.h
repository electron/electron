// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "base/callback.h"
#include "base/mac/scoped_nsobject.h"
#include "base/mac/scoped_sending_event.h"

@interface AtomApplication : NSApplication <CrAppProtocol,
                                            CrAppControlProtocol,
                                            NSUserActivityDelegate> {
 @private
  BOOL handlingSendEvent_;
  base::scoped_nsobject<NSUserActivity> currentActivity_ API_AVAILABLE(macosx(10.10));
  NSCondition* handoffLock_;
  BOOL updateReceived_;
  base::Callback<bool()> shouldShutdown_;
}

+ (AtomApplication*)sharedApplication;

- (void)setShutdownHandler:(base::Callback<bool()>)handler;

// CrAppProtocol:
- (BOOL)isHandlingSendEvent;

// CrAppControlProtocol:
- (void)setHandlingSendEvent:(BOOL)handlingSendEvent;

- (NSUserActivity*)getCurrentActivity API_AVAILABLE(macosx(10.10));
- (void)setCurrentActivity:(NSString*)type
              withUserInfo:(NSDictionary*)userInfo
            withWebpageURL:(NSURL*)webpageURL;
- (void)invalidateCurrentActivity;
- (void)updateCurrentActivity:(NSString*)type
                 withUserInfo:(NSDictionary*)userInfo;

@end
