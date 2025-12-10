// Copyright (c) 2024 Salesforce, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/api/electron_api_desktop_capturer.h"
#include "third_party/webrtc/modules/desktop_capture/desktop_capturer.h"

namespace electron::api {

bool DesktopCapturer::IsDisplayMediaSystemPickerAvailable() {
  if (webrtc::DesktopCapturer::IsRunningUnderWayland()) {
    return true;
  }
  return false;
}

}  // namespace electron::api
