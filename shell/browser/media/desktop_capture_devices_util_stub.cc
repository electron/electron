// Copyright (c) 2026 Anthropic, PBC.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "chrome/browser/media/webrtc/desktop_capture_devices_util.h"

#include "content/public/browser/desktop_media_id.h"

// Electron compiles NativeDesktopMediaList but not
// desktop_capture_devices_util.cc, which depends on the Chrome browser UI
// (BrowserCollection, TabSharingUI, ScreenCaptureNotificationUI). Upstream
// uses this to exclude the browser's own enterprise screenshot-protected
// windows from the picker. Electron has no equivalent enterprise policy, so
// no window is ever reported as blocked and desktopCapturer.getSources()
// keeps producing thumbnails for every window.
bool IsWindowSharingBlockedByDataProtection(
    const content::DesktopMediaID& media_id) {
  return false;
}
