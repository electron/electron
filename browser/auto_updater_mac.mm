// Copyright (c) 2013 GitHub, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "browser/auto_updater.h"

#import <ReactiveCocoa/RACCommand.h>
#import <ReactiveCocoa/RACSignal.h>
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
  }
}

// static
void AutoUpdater::CheckForUpdates() {
  RACSignal* signal = [g_updater.checkForUpdatesCommand execute:nil];

  AutoUpdaterDelegate* delegate = GetDelegate();
  if (!delegate)
    return;

  // Subscribe to events.
  __block bool has_update = false;
  [signal subscribeNext:^(SQRLDownloadedUpdate* downloadedUpdate) {
    has_update = true;

    // There is a new update that has been downloaded.
    SQRLUpdate* update = downloadedUpdate.update;
    delegate->OnUpdateDownloaded(
        base::SysNSStringToUTF8(update.releaseNotes),
        base::SysNSStringToUTF8(update.releaseName),
        base::Time::FromDoubleT(update.releaseDate.timeIntervalSince1970),
        base::SysNSStringToUTF8(update.updateURL.absoluteString),
        base::Bind(RelaunchToInstallUpdate));
  } error:^(NSError* error) {
    // Something wrong happened.
    delegate->OnError(base::SysNSStringToUTF8(error.localizedDescription));
  } completed:^() {
    // When the completed event is sent with no update, then we know there
    // is no update available.
    if (!has_update)
      delegate->OnUpdateNotAvailable();
    has_update = false;
  }];
}

}  // namespace auto_updater
