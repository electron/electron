// Copyright (c) 2013 GitHub, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "browser/auto_updater.h"

#import <ReactiveCocoa/RACCommand.h>
#import <ReactiveCocoa/RACSignal.h>
#import <ReactiveCocoa/NSObject+RACPropertySubscribing.h>
#import <Squirrel/Squirrel.h>

#include "base/bind.h"
#include "base/time/time.h"
#include "base/strings/sys_string_conversions.h"
#include "browser/auto_updater_delegate.h"

namespace auto_updater {

namespace {

// The gloal SQRLUpdater object.
SQRLUpdater* g_updater = nil;

void RelaunchToInstallUpdate() {
  [[g_updater relaunchToInstallUpdate] subscribeError:^(NSError* error) {
    AutoUpdaterDelegate* delegate = AutoUpdater::GetDelegate();
    if (delegate)
      delegate->OnError(base::SysNSStringToUTF8(error.localizedDescription));
  }];
}

}  // namespace

// static
void AutoUpdater::SetFeedURL(const std::string& feed) {
  if (g_updater == nil) {
    // Initialize the SQRLUpdater.
    NSURL* url = [NSURL URLWithString:base::SysUTF8ToNSString(feed)];
    NSURLRequest* urlRequest = [NSURLRequest requestWithURL:url];
    g_updater = [[SQRLUpdater alloc] initWithUpdateRequest:urlRequest];

    AutoUpdaterDelegate* delegate = GetDelegate();
    if (!delegate)
      return;

    [[g_updater rac_valuesForKeyPath:@"state" observer:g_updater]
      subscribeNext:^(NSNumber *stateNumber) {
        int state = [stateNumber integerValue];
        if (state == SQRLUpdaterStateCheckingForUpdate) {
          delegate->OnCheckingForUpdate();
        }
        else if (state == SQRLUpdaterStateDownloadingUpdate) {
          delegate->OnUpdateAvailable();
        }
    }];
  }
}

// static
void AutoUpdater::CheckForUpdates() {
  AutoUpdaterDelegate* delegate = GetDelegate();
  if (!delegate)
    return;

  [[[[g_updater.checkForUpdatesCommand
      execute:nil]
      // Send a `nil` after everything...
      concat:[RACSignal return:nil]]
      // But only take the first value. If an update is sent, we'll get that.
      // Otherwise, we'll get our inserted `nil` value.
      take:1]
      subscribeNext:^(SQRLDownloadedUpdate *downloadedUpdate) {
        if (downloadedUpdate) {
          SQRLUpdate* update = downloadedUpdate.update;
          // There is a new update that has been downloaded.
          delegate->OnUpdateDownloaded(
            base::SysNSStringToUTF8(update.releaseNotes),
            base::SysNSStringToUTF8(update.releaseName),
            base::Time::FromDoubleT(update.releaseDate.timeIntervalSince1970),
            base::SysNSStringToUTF8(update.updateURL.absoluteString),
            base::Bind(RelaunchToInstallUpdate));
        }
        else {
          // When the completed event is sent with no update, then we know there
          // is no update available.
          delegate->OnUpdateNotAvailable();
        }
      } error:^(NSError *error) {
        delegate->OnError(base::SysNSStringToUTF8(error.localizedDescription));
      }];
}
}  // namespace auto_updater
