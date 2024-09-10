// Copyright (c) 2024 Salesforce, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "electron/shell/browser/feature_list.h"

#include <string>

#include "base/dcheck_is_on.h"

namespace electron {

std::string EnablePlatformSpecificFeatures() {
  if (@available(macOS 14.4, *)) {
    // These flags aren't exported so reference them by name directly, they are
    // used to ensure that screen and window capture exclusive use
    // ScreenCaptureKit APIs to avoid warning dialogs on macOS 14.4 and higher.
    // kScreenCaptureKitPickerScreen,
    // chrome/browser/media/webrtc/thumbnail_capturer_mac.mm
    // kScreenCaptureKitStreamPickerSonoma,
    // chrome/browser/media/webrtc/thumbnail_capturer_mac.mm
    // kThumbnailCapturerMac,
    // chrome/browser/media/webrtc/thumbnail_capturer_mac.mm
#if DCHECK_IS_ON()
    return "ScreenCaptureKitPickerScreen,ScreenCaptureKitStreamPickerSonoma";
#else
    return "ScreenCaptureKitPickerScreen,ScreenCaptureKitStreamPickerSonoma,"
           "ThumbnailCapturerMac:capture_mode/sc_screenshot_manager";
#endif
  }
  if (@available(macOS 15.0, *)) {
    // These flags aren't exported so reference them by name directly, they are
    // used to ensure that screen and window capture exclusive use
    // ScreenCaptureKit APIs to avoid warning dialogs on macOS 15.0 and higher.
    // kScreenCaptureKitPickerScreen,
    // chrome/browser/media/webrtc/thumbnail_capturer_mac.mm
    // kScreenCaptureKitStreamPickerSonoma,
    // chrome/browser/media/webrtc/thumbnail_capturer_mac.mm
    // kThumbnailCapturerMac,
    // chrome/browser/media/webrtc/thumbnail_capturer_mac.mm
    // kUseSCContentSharingPicker,
    // chrome/browser/media/webrtc/thumbnail_capturer_mac.mm
#if DCHECK_IS_ON()
    return "ScreenCaptureKitPickerScreen,ScreenCaptureKitStreamPickerSonoma,"
           "UseSCContentSharingPicker";
#else
    return "ScreenCaptureKitPickerScreen,ScreenCaptureKitStreamPickerSonoma,"
           "UseSCContentSharingPicker,"
           "ThumbnailCapturerMac:capture_mode/sc_screenshot_manager";
#endif
  }
  return "";
}

}  // namespace electron
