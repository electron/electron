// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef SHELL_BROWSER_UI_WINDOW_ICON_UTILS_H_
#define SHELL_BROWSER_UI_WINDOW_ICON_UTILS_H_

#include "content/public/browser/desktop_media_id.h"
#include "ui/gfx/image/image_skia.h"

gfx::ImageSkia GetWindowIcon(content::DesktopMediaID id);

#endif  // SHELL_BROWSER_UI_WINDOW_ICON_UTILS_H_
