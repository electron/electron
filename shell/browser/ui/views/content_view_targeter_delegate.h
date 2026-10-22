// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_UI_VIEWS_CONTENT_VIEW_TARGETER_DELEGATE_H_
#define ELECTRON_SHELL_BROWSER_UI_VIEWS_CONTENT_VIEW_TARGETER_DELEGATE_H_

#include "ui/views/view_targeter_delegate.h"

namespace gfx {
class Rect;
}  // namespace gfx

namespace views {
class View;
}  // namespace views

namespace electron {

// A ViewTargeterDelegate for a window's content view that makes it
// hit-test-transparent except where one of its descendants covers the
// target rect.
//
// In BrowserWindow, the WebContentsView is added as a sibling of
// content_view_ at a lower z-order so that user-added child views paint
// above web content. content_view_ itself covers the full window, so the
// default DoesIntersectRect (a bounds check) would always return true and
// the parent targeter would stop there, never reaching the WebContentsView
// behind it. By only intersecting where a child actually does, the parent's
// default TargetForRect loop skips content_view_ and falls through to the
// sibling, ultimately resolving to the WebContentsView's NativeViewHost so
// BridgedContentView::hitTest: routes the event to RenderWidgetHostViewCocoa.
//
// This must never affect TargetForRect's contract — it only narrows
// HitTestRect, which the default loop already handles by skipping the child.
// Returning nullptr from TargetForRect (the previous approach) violated the
// contract and crashed RootView::UpdateCursor (see #51576).
class ContentViewTargeterDelegate : public views::ViewTargeterDelegate {
 public:
  bool DoesIntersectRect(const views::View* target,
                         const gfx::Rect& rect) const override;
};

}  // namespace electron

#endif  // ELECTRON_SHELL_BROWSER_UI_VIEWS_CONTENT_VIEW_TARGETER_DELEGATE_H_
