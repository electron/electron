// Copyright (c) 2022 Salesforce, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_DRAGGABLE_REGION_PROVIDER_H_
#define ELECTRON_SHELL_BROWSER_DRAGGABLE_REGION_PROVIDER_H_

namespace gfx {
class Point;
}

namespace views {
class View;
}

class DraggableRegionProvider {
 public:
  // The view that the provider's draggable regions lie in, if it has one.
  virtual views::View* GetDraggableRegionView() = 0;
  virtual int NonClientHitTest(const gfx::Point& point) = 0;
};

#endif  // ELECTRON_SHELL_BROWSER_DRAGGABLE_REGION_PROVIDER_H_
