// Copyright (c) 2020 Slack Technologies, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

// Copyright (c) 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_UI_GTK_STATUS_ICON_H_
#define ELECTRON_SHELL_BROWSER_UI_GTK_STATUS_ICON_H_

#include <memory>

#include "base/strings/string_util.h"
#include "ui/gfx/image/image_skia.h"
#include "ui/views/linux_ui/status_icon_linux.h"

namespace electron {

namespace gtkui {

bool IsStatusIconSupported();
std::unique_ptr<views::StatusIconLinux> CreateLinuxStatusIcon(
    const gfx::ImageSkia& image,
    const std::u16string& tool_tip,
    const char* id_prefix);

}  // namespace gtkui

}  // namespace electron

#endif  // ELECTRON_SHELL_BROWSER_UI_GTK_STATUS_ICON_H_
