// Copyright (c) 2026 Anthropic, PBC.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_OSR_OSR_DRAG_DELEGATE_H_
#define ELECTRON_SHELL_BROWSER_OSR_OSR_DRAG_DELEGATE_H_

#include "third_party/blink/public/common/page/drag_operation.h"
#include "ui/base/dragdrop/mojom/drag_drop_types.mojom-forward.h"

namespace gfx {
class ImageSkia;
class Vector2d;
}  // namespace gfx

namespace electron {

// Receives notifications about renderer-initiated drags in offscreen
// contents. The embedder drives the drag with synthetic input.
class OffScreenDragDelegate {
 public:
  virtual void OnOffScreenDragStart(const gfx::ImageSkia& image,
                                    const gfx::Vector2d& image_offset,
                                    blink::DragOperationsMask allowed_ops) = 0;
  virtual void OnOffScreenDragUpdate(ui::mojom::DragOperation operation) = 0;
  virtual void OnOffScreenDragEnd(ui::mojom::DragOperation operation,
                                  bool cancelled) = 0;

 protected:
  virtual ~OffScreenDragDelegate() = default;
};

}  // namespace electron

#endif  // ELECTRON_SHELL_BROWSER_OSR_OSR_DRAG_DELEGATE_H_
