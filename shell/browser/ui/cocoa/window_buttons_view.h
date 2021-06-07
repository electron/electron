// Copyright (c) 2021 Microsoft, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef SHELL_BROWSER_UI_COCOA_WINDOW_BUTTONS_VIEW_H_
#define SHELL_BROWSER_UI_COCOA_WINDOW_BUTTONS_VIEW_H_

#import <Cocoa/Cocoa.h>

#include "base/mac/scoped_nsobject.h"
#include "third_party/abseil-cpp/absl/types/optional.h"
#include "ui/gfx/geometry/point.h"

// Custom Quit, Minimize and Full Screen button container for frameless
// windows.
@interface WindowButtonsView : NSView {
 @private
  BOOL mouse_inside_;
  BOOL show_on_hover_;
  BOOL is_rtl_;
  gfx::Point margin_;
  base::scoped_nsobject<NSTrackingArea> tracking_area_;
}

- (id)initWithMargin:(const absl::optional<gfx::Point>&)margin;
- (void)setMargin:(const absl::optional<gfx::Point>&)margin;
- (void)setShowOnHover:(BOOL)yes;
- (void)setNeedsDisplayForButtons;
- (gfx::Point)getMargin;
@end

#endif  // SHELL_BROWSER_UI_COCOA_WINDOW_BUTTONS_VIEW_H_
