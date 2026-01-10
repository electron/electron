// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/ui/window_icon_utils.h"

#import <Cocoa/Cocoa.h>
#include "base/apple/foundation_util.h"
#include "base/apple/scoped_cftyperef.h"
#include "ui/gfx/image/image.h"
#include "ui/gfx/image/image_skia.h"
#include "ui/gfx/image/image_skia_util_mac.h"

// NOTE: This implementation is adapted from Chromium.
// The upstream Chromium implementation captures window icons at the default
// NSImage size (32x32), which results in blurry low-resolution image when
// converted to ImageSkia.
// We mirror the logic here but explicitly enforce a 128x128 size to capture
// high-DPI assets and ensure sharp icons in Electron's UI.
// Original source:
// https://source.chromium.org/chromium/chromium/src/+/main:chrome/browser/media/webrtc/window_icon_util_mac.mm;drc=bc5394765e49981ab69c744240cc5fb68f4b2a99
gfx::ImageSkia GetWindowIcon(content::DesktopMediaID id) {
  DCHECK(id.type == content::DesktopMediaID::TYPE_WINDOW);

  CGWindowID ids[1];
  ids[0] = id.id;

  base::apple::ScopedCFTypeRef<CFArrayRef> window_id_array(CFArrayCreate(
      nullptr, reinterpret_cast<const void**>(&ids), std::size(ids), nullptr));
  base::apple::ScopedCFTypeRef<CFArrayRef> window_array(
      CGWindowListCreateDescriptionFromArray(window_id_array.get()));

  if (!window_array || 0 == CFArrayGetCount(window_array.get())) {
    return gfx::ImageSkia();
  }

  CFDictionaryRef window = base::apple::CFCastStrict<CFDictionaryRef>(
      CFArrayGetValueAtIndex(window_array.get(), 0));
  CFNumberRef pid_ref = base::apple::GetValueFromDictionary<CFNumberRef>(
      window, kCGWindowOwnerPID);

  int pid;
  CFNumberGetValue(pid_ref, kCFNumberIntType, &pid);

  NSImage* icon_image =
      [[NSRunningApplication runningApplicationWithProcessIdentifier:pid] icon];

  if (!icon_image) {
    return gfx::ImageSkia();
  }

  return gfx::ImageSkiaFromResizedNSImage(icon_image, NSMakeSize(128, 128));
}
