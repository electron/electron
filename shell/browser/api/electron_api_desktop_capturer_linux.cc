// Copyright (c) 2026 byquanton
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/api/electron_api_desktop_capturer.h"
#include "third_party/webrtc/modules/desktop_capture/desktop_capturer.h"

#if defined(WEBRTC_USE_PIPEWIRE)
#include "third_party/webrtc/modules/desktop_capture/linux/wayland/base_capturer_pipewire.h"
#endif

namespace electron::api {

// static
bool DesktopCapturer::IsDisplayMediaSystemPickerAvailable() {
#if defined(WEBRTC_USE_PIPEWIRE)
  return webrtc::BaseCapturerPipeWire::IsSupported();
#else
  return false;
#endif
}

}  // namespace electron::api
