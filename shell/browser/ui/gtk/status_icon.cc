// Copyright (c) 2020 Slack Technologies, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

// Copyright (c) 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "shell/browser/ui/gtk/status_icon.h"

#include <gtk/gtk.h>

#include <memory>

#include "base/strings/stringprintf.h"
#include "shell/browser/ui/gtk/app_indicator_icon.h"
#include "shell/browser/ui/gtk/gtk_status_icon.h"

namespace electron::gtkui {

namespace {

int indicators_count = 0;

}

bool IsStatusIconSupported() {
#if GTK_CHECK_VERSION(3, 90, 0)
  NOTIMPLEMENTED();
  return false;
#else
  return true;
#endif
}

std::unique_ptr<ui::StatusIconLinux> CreateLinuxStatusIcon(
    const gfx::ImageSkia& image,
    const std::u16string& tool_tip,
    const char* id_prefix) {
#if GTK_CHECK_VERSION(3, 90, 0)
  NOTIMPLEMENTED();
  return nullptr;
#else
  if (AppIndicatorIcon::CouldOpen()) {
    ++indicators_count;

    return std::make_unique<AppIndicatorIcon>(
        base::StringPrintf("%s%d", id_prefix, indicators_count), image,
        tool_tip);
  } else {
    return std::make_unique<GtkStatusIcon>(image, tool_tip);
  }
#endif
}

}  // namespace electron::gtkui
