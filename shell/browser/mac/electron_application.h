// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef SHELL_BROWSER_MAC_ELECTRON_APPLICATION_H_
#define SHELL_BROWSER_MAC_ELECTRON_APPLICATION_H_

#include "base/callback.h"
#include "base/mac/scoped_nsobject.h"
#include "base/mac/scoped_sending_event.h"

#import <AVFoundation/AVFoundation.h>
#import <LocalAuthentication/LocalAuthentication.h>

@interface AtomApplication : NSApplication <CrAppProtocol,
                                            CrAppControlProtocol,
                                            NSUserActivityDelegate> {
 @private
  BOOL handlingSendEvent_;
  base::scoped_nsobject<NSUserActivity> currentActivity_;
  NSCondition* handoffLock_;
  BOOL updateReceived_;
  BOOL userStoppedShutdown_;
  base::RepeatingCallback<bool()> shouldShutdown_;
}

+ (AtomApplication*)sharedApplication;

- (void)setShutdownHandler:(base::RepeatingCallback<bool()>)handler;
- (void)registerURLHandler;

// Called when macOS itself is shutting down.
- (void)willPowerOff:(NSNotification*)notify;

// CrAppProtocol:
- (BOOL)isHandlingSendEvent;

// CrAppControlProtocol:
- (void)setHandlingSendEvent:(BOOL)handlingSendEvent;

- (NSUserActivity*)getCurrentActivity;
- (void)setCurrentActivity:(NSString*)type
              withUserInfo:(NSDictionary*)userInfo
            withWebpageURL:(NSURL*)webpageURL;
- (void)invalidateCurrentActivity;
- (void)resignCurrentActivity;
- (void)updateCurrentActivity:(NSString*)type
                 withUserInfo:(NSDictionary*)userInfo;

@end

#endif  // SHELL_BROWSER_MAC_ELECTRON_APPLICATION_H_
