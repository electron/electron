// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/icon_loader.h"

#include <windows.h>
#include <shellapi.h>

#include "base/bind.h"
#include "base/message_loop/message_loop.h"
#include "base/threading/thread.h"
#include "third_party/skia/include/core/SkBitmap.h"
#include "ui/display/win/dpi.h"
#include "ui/gfx/geometry/size.h"
#include "ui/gfx/icon_util.h"
#include "ui/gfx/image/image_skia.h"

// static
IconGroupID IconLoader::ReadGroupIDFromFilepath(
    const base::FilePath& filepath) {
  if (!IsIconMutableFromFilepath(filepath))
    return filepath.Extension();
  return filepath.value();
}

// static
bool IconLoader::IsIconMutableFromFilepath(const base::FilePath& filepath) {
  return filepath.MatchesExtension(L".exe") ||
         filepath.MatchesExtension(L".dll") ||
         filepath.MatchesExtension(L".ico");
}

// static
content::BrowserThread::ID IconLoader::ReadIconThreadID() {
  return content::BrowserThread::FILE;
}

void IconLoader::ReadIcon() {
  int size = 0;
  switch (icon_size_) {
    case IconLoader::SMALL:
      size = SHGFI_SMALLICON;
      break;
    case IconLoader::NORMAL:
      size = 0;
      break;
    case IconLoader::LARGE:
      size = SHGFI_LARGEICON;
      break;
    default:
      NOTREACHED();
  }

  image_.reset();

  SHFILEINFO file_info = {0};
  if (SHGetFileInfo(group_.c_str(), FILE_ATTRIBUTE_NORMAL, &file_info,
                    sizeof(SHFILEINFO),
                    SHGFI_ICON | size | SHGFI_USEFILEATTRIBUTES)) {
    std::unique_ptr<SkBitmap> bitmap(
        IconUtil::CreateSkBitmapFromHICON(file_info.hIcon));
    if (bitmap.get()) {
      gfx::ImageSkia image_skia(
          gfx::ImageSkiaRep(*bitmap, display::win::GetDPIScale()));
      image_skia.MakeThreadSafe();
      image_.reset(new gfx::Image(image_skia));
      DestroyIcon(file_info.hIcon);
    }
  }

  // Always notify the delegate, regardless of success.
  target_task_runner_->PostTask(FROM_HERE,
                                base::Bind(&IconLoader::NotifyDelegate, this));
}
