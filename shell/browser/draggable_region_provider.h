// Copyright (c) 2022 Salesforce, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_DRAGGABLE_REGION_PROVIDER_H_
#define ELECTRON_SHELL_BROWSER_DRAGGABLE_REGION_PROVIDER_H_

class DraggableRegionProvider {
 public:
  virtual int NonClientHitTest(const gfx::Point& point) = 0;
};

#endif  // ELECTRON_SHELL_BROWSER_DRAGGABLE_REGION_PROVIDER_H_
