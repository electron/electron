// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/icon_loader.h"

#include "base/bind.h"
#include "base/message_loop/message_loop.h"
#include "base/nix/mime_util_xdg.h"
#include "ui/views/linux_ui/linux_ui.h"

// static
IconLoader::IconGroup IconLoader::GroupForFilepath(
    const base::FilePath& file_path) {
  return base::nix::GetFileMimeType(file_path);
}

// static
content::BrowserThread::ID IconLoader::ReadIconThreadID() {
  // ReadIcon() calls into views::LinuxUI and GTK2 code, so it must be on the UI
  // thread.
  return content::BrowserThread::UI;
}

void IconLoader::ReadIcon() {
  int size_pixels = 0;
  switch (icon_size_) {
    case IconLoader::SMALL:
      size_pixels = 16;
      break;
    case IconLoader::NORMAL:
      size_pixels = 32;
      break;
    case IconLoader::LARGE:
      size_pixels = 48;
      break;
    default:
      NOTREACHED();
  }

  views::LinuxUI* ui = views::LinuxUI::instance();
  if (ui) {
    gfx::Image image = ui->GetIconForContentType(group_, size_pixels);
    if (!image.IsEmpty())
      image_.reset(new gfx::Image(image));
  }

  target_task_runner_->PostTask(
      FROM_HERE, base::Bind(callback_, base::Passed(&image_), group_));
  delete this;
}
