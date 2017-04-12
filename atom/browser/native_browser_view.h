// Copyright (c) 2017 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_NATIVE_BROWSER_VIEW_H_
#define ATOM_BROWSER_NATIVE_BROWSER_VIEW_H_

#include "base/macros.h"
#include "third_party/skia/include/core/SkColor.h"

namespace brightray {
class InspectableWebContentsView;
}

namespace gfx {
class Rect;
}

namespace atom {

namespace api {
class WebContents;
}

enum AutoResizeFlags {
  kAutoResizeWidth = 0x1,
  kAutoResizeHeight = 0x2,
};

class NativeBrowserView {
 public:
  virtual ~NativeBrowserView();

  static NativeBrowserView* Create(
      brightray::InspectableWebContentsView* web_contents_view);

  brightray::InspectableWebContentsView* GetInspectableWebContentsView() {
    return web_contents_view_;
  }

  virtual void SetAutoResizeFlags(uint8_t flags) = 0;
  virtual void SetBounds(const gfx::Rect& bounds) = 0;
  virtual void SetBackgroundColor(SkColor color) = 0;

 protected:
  explicit NativeBrowserView(
      brightray::InspectableWebContentsView* web_contents_view);

  brightray::InspectableWebContentsView* web_contents_view_;

 private:
  DISALLOW_COPY_AND_ASSIGN(NativeBrowserView);
};

}  // namespace atom

#endif  // ATOM_BROWSER_NATIVE_BROWSER_VIEW_H_
