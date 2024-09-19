// Copyright (c) 2016 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_UI_DRAG_UTIL_H_
#define ELECTRON_SHELL_BROWSER_UI_DRAG_UTIL_H_

#include <memory>
#include <vector>

#include "third_party/blink/public/mojom/page/draggable_region.mojom-forward.h"
#include "ui/gfx/native_widget_types.h"

class SkRegion;

namespace base {
class FilePath;
}

namespace gfx {
class Image;
}

namespace electron {

void DragFileItems(const std::vector<base::FilePath>& files,
                   const gfx::Image& icon,
                   gfx::NativeView view);

// Convert draggable regions in raw format to SkRegion format.
std::unique_ptr<SkRegion> DraggableRegionsToSkRegion(
    const std::vector<blink::mojom::DraggableRegionPtr>& regions);

}  // namespace electron

#endif  // ELECTRON_SHELL_BROWSER_UI_DRAG_UTIL_H_
