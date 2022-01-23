#include "shell/browser/ui/native_view.h"

#include "base/mac/foundation_util.h"
#include "base/mac/scoped_cftyperef.h"
#include "shell/browser/ui/cocoa/electron_native_view.h"
#include "ui/gfx/geometry/rect_conversions.h"
#include "ui/gfx/geometry/rect_f.h"

namespace electron {

void NativeView::SetNativeView(NATIVEVIEW view) {
  view_ = view;

  if (!IsNativeView(view))
    return;

  Class cl = [view class];
  if (!NativeViewMethodsInstalled(cl)) {
    InstallNativeViewMethods(cl);
  }

  NativeViewPrivate* priv = [view nativeViewPrivate];
  priv->shell = this;

  SEL cmd = @selector(acceptsFirstResponder);
  auto super_impl = reinterpret_cast<BOOL (*)(NSView*, SEL)>(
      [[view superclass] instanceMethodForSelector:cmd]);
  priv->focusable = super_impl(view, cmd);
}

void NativeView::InitView() {
  SetNativeView([[ElectronNativeView alloc] init]);
}

void NativeView::DestroyView() {
  if (IsNativeView(view_)) {
    NativeViewPrivate* priv = [view_ nativeViewPrivate];
    priv->shell = nullptr;
  }
  [view_ release];
}

void NativeView::SetBounds(const gfx::Rect& bounds) {
  NSRect frame = bounds.ToCGRect();
  auto* superview = [view_ superview];
  if (superview && ![superview isFlipped]) {
    const auto superview_height = superview.frame.size.height;
    frame =
        NSMakeRect(bounds.x(), superview_height - bounds.y() - bounds.height(),
                   bounds.width(), bounds.height());
  }
  [view_ setFrame:frame];
  [view_ resizeSubviewsWithOldSize:frame.size];
}

gfx::Rect NativeView::GetBounds() const {
  return ToNearestRect(gfx::RectF([view_ frame]));
}

gfx::Point NativeView::OffsetFromView(const NativeView* from) const {
  NSPoint point = [view_ convertPoint:NSZeroPoint toView:from->view_];
  return gfx::Point(point.x, point.y);
}

gfx::Point NativeView::OffsetFromWindow() const {
  NSPoint point = [view_ convertPoint:NSZeroPoint toView:nil];
  return gfx::Point(point.x, point.y);
}

void NativeView::SetVisibleImpl(bool visible) {
  [view_ setHidden:!visible];
}

bool NativeView::IsVisible() const {
  return ![view_ isHidden];
}

bool NativeView::IsTreeVisible() const {
  return ![view_ isHiddenOrHasHiddenAncestor];
}

void NativeView::Focus() {
  if (view_.window && IsFocusable())
    [view_.window makeFirstResponder:view_];
}

bool NativeView::HasFocus() const {
  if (view_.window)
    return view_.window.firstResponder == view_;
  else
    return false;
}

void NativeView::SetFocusable(bool focusable) {
  NativeViewPrivate* priv = [view_ nativeViewPrivate];
  priv->focusable = focusable;
}

bool NativeView::IsFocusable() const {
  return [view_ acceptsFirstResponder];
}

void NativeView::SetBackgroundColor(SkColor color) {
  if (IsNativeView(view_))
    [view_ setNativeBackgroundColor:color];
}

void NativeView::SetWantsLayer(bool wants) {
  [view_ nativeViewPrivate]->wants_layer = wants;
  [view_ setWantsLayer:wants];
}

bool NativeView::WantsLayer() const {
  return [view_ wantsLayer];
}

void NativeView::UpdateDraggableRegions() {}

}  // namespace electron
