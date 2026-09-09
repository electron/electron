// Copyright (c) 2024 Salesforce, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "electron/shell/browser/feature_list.h"

#include <string>

#include "base/dcheck_is_on.h"

namespace electron {

std::string EnablePlatformSpecificFeatures() {
  // None of these flags are exported, so they are referenced by name.

  // Throttle visual-property IPCs during live resize (kThrottleResizeIpc in
  // content/browser/renderer_host/render_widget_host_view_mac.mm).
  std::string features = "ThrottleResizeIpc";
  if (@available(macOS 14.4, *)) {
    // Make screen and window capture use ScreenCaptureKit APIs exclusively to
    // avoid warning dialogs on macOS 14.4 and higher.
    // kScreenCaptureKitPickerScreen, kScreenCaptureKitStreamPickerSonoma,
    // kThumbnailCapturerMac,
    // chrome/browser/media/webrtc/thumbnail_capturer_mac.mm
    features +=
        ",ScreenCaptureKitPickerScreen,ScreenCaptureKitStreamPickerSonoma";
#if !DCHECK_IS_ON()
    features += ",ThumbnailCapturerMac:capture_mode/sc_screenshot_manager";
#endif
  }
  return features;
}

std::string DisablePlatformSpecificFeatures() {
  if (@available(macOS 14.4, *)) {
    // Required to stop timing out getDisplayMedia while waiting for
    // the user to select a window with the picker
    return "TimeoutHangingVideoCaptureStarts";
  }
  return "";
}

}  // namespace electron
