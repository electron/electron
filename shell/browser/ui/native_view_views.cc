#include "shell/browser/ui/native_view.h"

#include "ui/gfx/geometry/rect_conversions.h"
#include "ui/views/background.h"
#include "ui/views/view.h"

namespace electron {

void NativeView::SetNativeView(NATIVEVIEW view) {
  if (view_) {
    view_->RemoveObserver(this);
    if (delete_view_)
      delete view_;
  }
  view_ = view;
  view_->AddObserver(this);
}

void NativeView::InitView() {
  SetNativeView(new views::View());
}

void NativeView::DestroyView() {
  if (!view_)
    return;
  view_->RemoveObserver(this);
  if (delete_view_)
    delete view_;
}

void NativeView::OnViewBoundsChanged(views::View* observed_view) {
  if (!view_)
    return;
  gfx::Rect bounds = view_->bounds();
  gfx::Size size = bounds.size();
  gfx::Size old_size = bounds_.size();
  bounds_ = bounds;
  if (size != old_size)
    NotifySizeChanged(old_size, size);
}

void NativeView::OnViewIsDeleting(views::View* observed_view) {
  view_ = nullptr;
  NotifyViewIsDeleting();
}

void NativeView::SetBounds(const gfx::Rect& bounds) {
  if (view_)
    view_->SetBoundsRect(bounds);
}

gfx::Rect NativeView::GetBounds() const {
  if (view_)
    return view_->bounds();
  return gfx::Rect();
}

gfx::Point NativeView::OffsetFromView(const NativeView* from) const {
  if (!view_)
    return gfx::Point();
  gfx::Point point;
  views::View::ConvertPointToTarget(from->GetNative(), view_, &point);
  return point;
}

gfx::Point NativeView::OffsetFromWindow() const {
  if (!view_)
    return gfx::Point();
  gfx::Point point;
  views::View::ConvertPointFromWidget(view_, &point);
  return point;
}

void NativeView::SetVisibleImpl(bool visible) {
  if (view_)
    view_->SetVisible(visible);
}

bool NativeView::IsVisible() const {
  if (view_)
    return view_->GetVisible();
  return false;
}

bool NativeView::IsTreeVisible() const {
  return IsVisible();
}

void NativeView::Focus() {
  if (view_)
    view_->RequestFocus();
}

bool NativeView::HasFocus() const {
  if (view_)
    return view_->HasFocus();
  return false;
}

void NativeView::SetFocusable(bool focusable) {}

bool NativeView::IsFocusable() const {
  if (view_)
    return view_->IsFocusable();
  return false;
}

void NativeView::SetBackgroundColor(SkColor color) {
  if (!view_)
    return;
  view_->SetBackground(views::CreateSolidBackground(color));
  view_->SchedulePaint();
}

}  // namespace electron
