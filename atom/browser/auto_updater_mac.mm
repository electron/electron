// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/auto_updater.h"

#import <ReactiveCocoa/RACCommand.h>
#import <ReactiveCocoa/RACSignal.h>
#import <ReactiveCocoa/NSObject+RACPropertySubscribing.h>
#import <Squirrel/Squirrel.h>

#include "base/bind.h"
#include "base/time/time.h"
#include "base/strings/sys_string_conversions.h"

namespace auto_updater {

namespace {

// The gloal SQRLUpdater object.
SQRLUpdater* g_updater = nil;

}  // namespace

namespace {

bool g_update_available = false;
std::string update_url_ = "";

}

std::string AutoUpdater::GetFeedURL() {
  return update_url_;
}

// static
void AutoUpdater::SetFeedURL(const std::string& feed,
                             const HeaderMap& requestHeaders) {
  Delegate* delegate = GetDelegate();
  if (!delegate)
    return;

  update_url_ = feed;

  NSURL* url = [NSURL URLWithString:base::SysUTF8ToNSString(feed)];
  NSMutableURLRequest* urlRequest = [NSMutableURLRequest requestWithURL:url];

  for (const auto& it : requestHeaders) {
    [urlRequest setValue:base::SysUTF8ToNSString(it.second)
      forHTTPHeaderField:base::SysUTF8ToNSString(it.first)];
  }

  if (g_updater)
    [g_updater release];

  // Initialize the SQRLUpdater.
  @try {
    g_updater = [[SQRLUpdater alloc] initWithUpdateRequest:urlRequest];
  } @catch (NSException* error) {
    delegate->OnError(base::SysNSStringToUTF8(error.reason));
    return;
  }

  [[g_updater rac_valuesForKeyPath:@"state" observer:g_updater]
    subscribeNext:^(NSNumber *stateNumber) {
      int state = [stateNumber integerValue];
      // Dispatching the event on main thread.
      dispatch_async(dispatch_get_main_queue(), ^{
        if (state == SQRLUpdaterStateCheckingForUpdate)
          delegate->OnCheckingForUpdate();
        else if (state == SQRLUpdaterStateDownloadingUpdate)
          delegate->OnUpdateAvailable();
      });
  }];
}

// static
void AutoUpdater::CheckForUpdates() {
  Delegate* delegate = GetDelegate();
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
          g_update_available = true;
          SQRLUpdate* update = downloadedUpdate.update;
          // There is a new update that has been downloaded.
          delegate->OnUpdateDownloaded(
            base::SysNSStringToUTF8(update.releaseNotes),
            base::SysNSStringToUTF8(update.releaseName),
            base::Time::FromDoubleT(update.releaseDate.timeIntervalSince1970),
            base::SysNSStringToUTF8(update.updateURL.absoluteString));
        } else {
          g_update_available = false;
          // When the completed event is sent with no update, then we know there
          // is no update available.
          delegate->OnUpdateNotAvailable();
        }
      } error:^(NSError *error) {
        NSString* failureString = error.localizedFailureReason ?
            [NSString stringWithFormat:@"%@: %@",
                                       error.localizedDescription,
                                       error.localizedFailureReason] :
            [NSString stringWithString:error.localizedDescription];
        delegate->OnError(base::SysNSStringToUTF8(failureString));
      }];
}

void AutoUpdater::QuitAndInstall() {
  Delegate* delegate = AutoUpdater::GetDelegate();
  if (g_update_available) {
    [[g_updater relaunchToInstallUpdate] subscribeError:^(NSError* error) {
      if (delegate)
        delegate->OnError(base::SysNSStringToUTF8(error.localizedDescription));
    }];
  } else {
    if (delegate)
      delegate->OnError("No update available, can't quit and install");
  }
}

}  // namespace auto_updater
