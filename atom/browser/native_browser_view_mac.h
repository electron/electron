// Copyright (c) 2017 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_NATIVE_BROWSER_VIEW_MAC_H_
#define ATOM_BROWSER_NATIVE_BROWSER_VIEW_MAC_H_

#import <Cocoa/Cocoa.h>

#include "atom/browser/native_browser_view.h"

namespace atom {

class NativeBrowserViewMac : public NativeBrowserView {
 public:
  explicit NativeBrowserViewMac(
      brightray::InspectableWebContentsView* web_contents_view);
  ~NativeBrowserViewMac() override;

  void SetAutoResizeFlags(uint8_t flags) override;
  void SetBounds(const gfx::Rect& bounds) override;
  void SetBackgroundColor(SkColor color) override;

 private:
  DISALLOW_COPY_AND_ASSIGN(NativeBrowserViewMac);
};

}  // namespace atom

#endif  // ATOM_BROWSER_NATIVE_BROWSER_VIEW_MAC_H_
