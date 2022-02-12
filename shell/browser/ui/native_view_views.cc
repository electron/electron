// Copyright (c) 2022 GitHub, Inc.

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
  delete_view_ = true;
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

void NativeView::SetBackgroundColor(SkColor color) {
  if (!view_)
    return;
  view_->SetBackground(views::CreateSolidBackground(color));
  view_->SchedulePaint();
}

}  // namespace electron
