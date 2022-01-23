#include "shell/browser/ui/native_scroll_view.h"

#include <utility>

#include "shell/browser/ui/native_container_view.h"
#include "ui/gfx/geometry/size_conversions.h"

namespace electron {

NativeScrollView::NativeScrollView() {
  InitScrollView();
  SetContentView(new NativeContainerView);
}

NativeScrollView::~NativeScrollView() = default;

void NativeScrollView::SetContentView(scoped_refptr<NativeView> view) {
  if (content_view_)
    content_view_->SetParent(nullptr);
  SetContentViewImpl(view.get());
  content_view_ = std::move(view);
  content_view_->SetParent(this);
}

NativeView* NativeScrollView::GetContentView() const {
  return content_view_.get();
}

gfx::Size NativeScrollView::GetContentSize() const {
  return GetContentView()->GetBounds().size();
}

void NativeScrollView::DetachChildView(NativeView* view) {
  if (!view || content_view_.get() != view)
    return;
  DetachChildViewImpl();
  content_view_->SetParent(nullptr);
  content_view_.reset();
  NotifyChildViewDetached(view);
}

void NativeScrollView::TriggerBeforeunloadEvents() {
  if (content_view_.get())
    content_view_->TriggerBeforeunloadEvents();
}

void NativeScrollView::SetWindowForChildren(NativeWindow* window) {
  if (content_view_.get())
    content_view_->SetWindow(window);
}

}  // namespace electron
