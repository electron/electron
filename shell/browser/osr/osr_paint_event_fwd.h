// Copyright (c) 2024 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_OSR_OSR_PAINT_EVENT_FWD_H_
#define ELECTRON_SHELL_BROWSER_OSR_OSR_PAINT_EVENT_FWD_H_

#include <optional>

#include "base/functional/callback_forward.h"

class SkBitmap;

namespace gfx {
class Rect;
}  // namespace gfx

namespace electron {

struct OffscreenSharedTextureValue;

typedef std::optional<OffscreenSharedTextureValue> OffscreenSharedTexture;

typedef base::RepeatingCallback<
    void(const gfx::Rect&, const SkBitmap&, const OffscreenSharedTexture&)>
    OnPaintCallback;

}  // namespace electron

#endif  // ELECTRON_SHELL_BROWSER_OSR_OSR_PAINT_EVENT_FWD_H_
