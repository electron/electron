// Copyright (c) 2013 GitHub, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "browser/auto_updater.h"

// Sparkle's headers are throwing compilation warnings, supress them.
#pragma GCC diagnostic ignored "-Wmissing-method-return-type"
#import <Sparkle/Sparkle.h>

#include "base/bind.h"
#include "base/memory/scoped_ptr.h"
#include "base/strings/sys_string_conversions.h"
#include "browser/auto_updater_delegate.h"

using auto_updater::AutoUpdaterDelegate;

namespace {

struct NSInvocationDeleter {
  inline void operator()(NSInvocation* invocation) const {
    [invocation release];
  }
};

typedef scoped_ptr<NSInvocation, NSInvocationDeleter> ScopedNSInvocation;

// We are passing the NSInvocation as scoped_ptr, because we want to make sure
// whether or not the callback is called, the NSInvocation should alwasy be
// released, the only way to ensure it is to use scoped_ptr.
void CallNSInvocation(ScopedNSInvocation invocation) {
  [invocation.get() invoke];
}

}  // namespace

@interface SUUpdaterDelegate : NSObject {
}
@end

@implementation SUUpdaterDelegate

- (BOOL)updater:(SUUpdater*)updater
        shouldPostponeRelaunchForUpdate:(SUAppcastItem*)update
        untilInvoking:(NSInvocation*)invocation {
  AutoUpdaterDelegate* delegate = auto_updater::AutoUpdater::GetDelegate();
  if (!delegate)
    return NO;

  std::string version(base::SysNSStringToUTF8([update versionString]));
  ScopedNSInvocation invocation_ptr([invocation retain]);
  delegate->WillInstallUpdate(
      version,
      base::Bind(&CallNSInvocation, base::Passed(invocation_ptr.Pass())));

  return YES;
}

- (void)updater:(SUUpdater*)updater
        willInstallUpdateOnQuit:(SUAppcastItem*)update
        immediateInstallationInvocation:(NSInvocation*)invocation {
  AutoUpdaterDelegate* delegate = auto_updater::AutoUpdater::GetDelegate();
  if (!delegate)
    return;

  std::string version(base::SysNSStringToUTF8([update versionString]));
  ScopedNSInvocation invocation_ptr([invocation retain]);
  delegate->ReadyForUpdateOnQuit(
      version,
      base::Bind(&CallNSInvocation, base::Passed(invocation_ptr.Pass())));
}

@end

namespace auto_updater {

// static
void AutoUpdater::Init() {
  SUUpdaterDelegate* delegate = [[SUUpdaterDelegate alloc] init];
  [[SUUpdater sharedUpdater] setDelegate:delegate];
}

// static
void AutoUpdater::SetFeedURL(const std::string& url) {
  NSString* url_str(base::SysUTF8ToNSString(url));
  [[SUUpdater sharedUpdater] setFeedURL:[NSURL URLWithString:url_str]];
}

// static
void AutoUpdater::SetAutomaticallyChecksForUpdates(bool yes) {
  [[SUUpdater sharedUpdater] setAutomaticallyChecksForUpdates:yes];
}

// static
void AutoUpdater::SetAutomaticallyDownloadsUpdates(bool yes) {
  [[SUUpdater sharedUpdater] setAutomaticallyDownloadsUpdates:yes];
}

// static
void AutoUpdater::CheckForUpdates() {
  [[SUUpdater sharedUpdater] checkForUpdates:nil];
}

// static
void AutoUpdater::CheckForUpdatesInBackground() {
  [[SUUpdater sharedUpdater] checkForUpdatesInBackground];
}

}  // namespace auto_updater
