// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/api/atom_api_power_monitor.h"

#include <vector>

#import <ApplicationServices/ApplicationServices.h>
#import <Cocoa/Cocoa.h>

@interface MacLockMonitor : NSObject {
 @private
  std::vector<electron::api::PowerMonitor*> emitters;
}

- (void)addEmitter:(electron::api::PowerMonitor*)monitor_;

@end

@implementation MacLockMonitor

- (id)init {
  if ((self = [super init])) {
    NSDistributedNotificationCenter* distCenter =
        [NSDistributedNotificationCenter defaultCenter];
    [distCenter addObserver:self
                   selector:@selector(onScreenLocked:)
                       name:@"com.apple.screenIsLocked"
                     object:nil];
    [distCenter addObserver:self
                   selector:@selector(onScreenUnlocked:)
                       name:@"com.apple.screenIsUnlocked"
                     object:nil];
  }
  return self;
}

- (void)dealloc {
  [[NSDistributedNotificationCenter defaultCenter] removeObserver:self];
  [super dealloc];
}

- (void)addEmitter:(electron::api::PowerMonitor*)monitor_ {
  self->emitters.push_back(monitor_);
}

- (void)onScreenLocked:(NSNotification*)notification {
  for (auto*& emitter : self->emitters) {
    emitter->Emit("lock-screen");
  }
}

- (void)onScreenUnlocked:(NSNotification*)notification {
  for (auto*& emitter : self->emitters) {
    emitter->Emit("unlock-screen");
  }
}

@end

namespace electron {

namespace api {

static MacLockMonitor* g_lock_monitor = nil;

void PowerMonitor::InitPlatformSpecificMonitors() {
  if (!g_lock_monitor)
    g_lock_monitor = [[MacLockMonitor alloc] init];
  [g_lock_monitor addEmitter:this];
}

}  // namespace api

}  // namespace electron
