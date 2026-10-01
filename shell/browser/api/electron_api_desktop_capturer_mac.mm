// Copyright (c) 2024 Salesforce, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/api/electron_api_desktop_capturer.h"

#import <AppKit/AppKit.h>
#import <ScreenCaptureKit/ScreenCaptureKit.h>

namespace electron::api {

// static
bool DesktopCapturer::IsDisplayMediaSystemPickerAvailable() {
  if (@available(macOS 15.0, *)) {
    return true;
  }
  return false;
}

// static
void DesktopCapturer::ExcludeContentProtectedWindowsFromSystemPicker() {
  if (@available(macOS 15.0, *)) {
    NSMutableArray<NSNumber*>* excluded = [NSMutableArray array];
    for (NSWindow* window in [NSApp windows]) {
      if (window.sharingType == NSWindowSharingNone)
        [excluded addObject:@(window.windowNumber)];
    }
    // Upstream's Open() reads defaultConfiguration, adjusts it and writes it
    // back, so this list survives into the session it starts.
    SCContentSharingPicker* picker = [SCContentSharingPicker sharedPicker];
    SCContentSharingPickerConfiguration* config = picker.defaultConfiguration;
    config.excludedWindowIDs = excluded;
    picker.defaultConfiguration = config;
  }
}

}  // namespace electron::api
