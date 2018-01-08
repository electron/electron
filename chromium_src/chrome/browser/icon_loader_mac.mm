// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/icon_loader.h"

#import <AppKit/AppKit.h>

#include "base/bind.h"
#include "base/files/file_path.h"
#include "base/memory/ptr_util.h"
#include "base/message_loop/message_loop.h"
#include "base/strings/sys_string_conversions.h"
#include "base/threading/thread.h"
#include "ui/gfx/image/image_skia.h"
#include "ui/gfx/image/image_skia_util_mac.h"

// static
IconLoader::IconGroup IconLoader::GroupForFilepath(
    const base::FilePath& file_path) {
  return file_path.Extension();
}

// static
content::BrowserThread::ID IconLoader::ReadIconThreadID() {
  return content::BrowserThread::FILE;
}

void IconLoader::ReadIcon() {
  NSString* group = base::SysUTF8ToNSString(group_);
  NSWorkspace* workspace = [NSWorkspace sharedWorkspace];
  NSImage* icon = [workspace iconForFileType:group];

  std::unique_ptr<gfx::Image> image;

  if (icon_size_ == ALL) {
    // The NSImage already has all sizes.
    image = base::MakeUnique<gfx::Image>([icon retain]);
  } else {
    NSSize size = NSZeroSize;
    switch (icon_size_) {
      case IconLoader::SMALL:
        size = NSMakeSize(16, 16);
        break;
      case IconLoader::NORMAL:
        size = NSMakeSize(32, 32);
        break;
      default:
        NOTREACHED();
    }
    gfx::ImageSkia image_skia(gfx::ImageSkiaFromResizedNSImage(icon, size));
    if (!image_skia.isNull()) {
      image_skia.MakeThreadSafe();
      image = base::MakeUnique<gfx::Image>(image_skia);
    }
  }

  target_task_runner_->PostTask(
      FROM_HERE, base::Bind(callback_, base::Passed(&image), group_));
  delete this;
}
