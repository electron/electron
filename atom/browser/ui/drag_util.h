// Copyright (c) 2016 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_UI_DRAG_UTIL_H_
#define ATOM_BROWSER_UI_DRAG_UTIL_H_

#include <vector>

#include "ui/gfx/image/image.h"

namespace base {
class FilePath;
}

namespace atom {

void DragFileItems(const std::vector<base::FilePath>& files,
                   const gfx::Image& icon,
                   gfx::NativeView view);

}  // namespace atom

#endif  // ATOM_BROWSER_UI_DRAG_UTIL_H_
