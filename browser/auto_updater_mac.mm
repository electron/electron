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
static SQRLUpdater* g_updater = nil;

static void RelaunchToInstallUpdate() {
  if (g_updater != nil)
    [g_updater relaunchToInstallUpdate];
}

}  // namespace

// static
void AutoUpdater::SetFeedURL(const std::string& feed) {
  if (g_updater == nil) {
    // Initialize the SQRLUpdater.
    NSURL* url = [NSURL URLWithString:base::SysUTF8ToNSString(feed)];
    NSURLRequest* urlRequest = [NSURLRequest requestWithURL:url];
    g_updater = [[SQRLUpdater alloc] initWithUpdateRequest:urlRequest];

    // Subscribe to events.
    [g_updater.updates subscribeNext:^(SQRLDownloadedUpdate* downloadedUpdate) {
       AutoUpdaterDelegate* delegate = GetDelegate();
       if (!delegate)
         return;

       SQRLUpdate* update = downloadedUpdate.update;
       delegate->OnUpdateDownloaded(
           base::SysNSStringToUTF8(update.releaseNotes),
           base::SysNSStringToUTF8(update.releaseName),
           base::Time::FromDoubleT(update.releaseDate.timeIntervalSince1970),
           base::SysNSStringToUTF8(update.updateURL.absoluteString),
           base::Bind(RelaunchToInstallUpdate));
    }];
  }
}

// static
void AutoUpdater::CheckForUpdates() {
  if (g_updater != nil) {
    [g_updater.checkForUpdatesCommand execute:nil];
  }
}

}  // namespace auto_updater
