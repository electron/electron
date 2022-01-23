#include "shell/browser/ui/native_scroll_view.h"

#include "ui/views/controls/scroll_view.h"

namespace electron {

namespace {

ScrollBarMode GetScrollBarMode(views::ScrollView::ScrollBarMode mode) {
  switch (mode) {
    case views::ScrollView::ScrollBarMode::kDisabled:
      return ScrollBarMode::kDisabled;
    case views::ScrollView::ScrollBarMode::kHiddenButEnabled:
      return ScrollBarMode::kHiddenButEnabled;
    case views::ScrollView::ScrollBarMode::kEnabled:
      return ScrollBarMode::kEnabled;
  }
}

}  // namespace

void NativeScrollView::InitScrollView() {
  SetNativeView(new views::ScrollView());
}

void NativeScrollView::SetContentViewImpl(NativeView* view) {
  if (!GetNative() || !view)
    return;
  auto* scroll = static_cast<views::ScrollView*>(GetNative());
  view->set_delete_view(false);
  auto content_view = std::unique_ptr<views::View>(view->GetNative());
  scroll->SetContents(std::move(content_view));
}

void NativeScrollView::DetachChildViewImpl() {
  if (!GetNative())
    return;
  auto* scroll = static_cast<views::ScrollView*>(GetNative());
  scroll->SetContents(nullptr);
  content_view_->set_delete_view(true);
}

void NativeScrollView::SetContentSize(const gfx::Size& size) {
  if (!content_view_.get())
    return;
  content_view_->GetNative()->SetSize(size);
}

void NativeScrollView::SetHorizontalScrollBarMode(ScrollBarMode mode) {
  if (!GetNative())
    return;
  auto* scroll = static_cast<views::ScrollView*>(GetNative());
  views::ScrollView::ScrollBarMode scroll_bar_mode =
      views::ScrollView::ScrollBarMode::kEnabled;
  switch (mode) {
    case ScrollBarMode::kDisabled:
      scroll_bar_mode = views::ScrollView::ScrollBarMode::kDisabled;
      break;
    case ScrollBarMode::kHiddenButEnabled:
      scroll_bar_mode = views::ScrollView::ScrollBarMode::kHiddenButEnabled;
      break;
    case ScrollBarMode::kEnabled:
      scroll_bar_mode = views::ScrollView::ScrollBarMode::kEnabled;
  }
  scroll->SetHorizontalScrollBarMode(scroll_bar_mode);
}

ScrollBarMode NativeScrollView::GetHorizontalScrollBarMode() const {
  if (!GetNative())
    return ScrollBarMode::kDisabled;
  auto* scroll = static_cast<views::ScrollView*>(GetNative());
  return GetScrollBarMode(scroll->GetHorizontalScrollBarMode());
}

void NativeScrollView::SetVerticalScrollBarMode(ScrollBarMode mode) {
  if (!GetNative())
    return;
  auto* scroll = static_cast<views::ScrollView*>(GetNative());
  views::ScrollView::ScrollBarMode scroll_bar_mode =
      views::ScrollView::ScrollBarMode::kEnabled;
  switch (mode) {
    case ScrollBarMode::kDisabled:
      scroll_bar_mode = views::ScrollView::ScrollBarMode::kDisabled;
      break;
    case ScrollBarMode::kHiddenButEnabled:
      scroll_bar_mode = views::ScrollView::ScrollBarMode::kHiddenButEnabled;
      break;
    case ScrollBarMode::kEnabled:
      scroll_bar_mode = views::ScrollView::ScrollBarMode::kEnabled;
  }
  scroll->SetVerticalScrollBarMode(scroll_bar_mode);
}

ScrollBarMode NativeScrollView::GetVerticalScrollBarMode() const {
  if (!GetNative())
    return ScrollBarMode::kDisabled;
  auto* scroll = static_cast<views::ScrollView*>(GetNative());
  return GetScrollBarMode(scroll->GetVerticalScrollBarMode());
}

void NativeScrollView::ClipHeightTo(int min_height, int max_height) {
  if (!GetNative())
    return;
  auto* scroll = static_cast<views::ScrollView*>(GetNative());
  scroll->ClipHeightTo(min_height, max_height);
}

int NativeScrollView::GetMinHeight() const {
  if (!GetNative())
    return 0;
  auto* scroll = static_cast<views::ScrollView*>(GetNative());
  return scroll->GetMinHeight();
}

int NativeScrollView::GetMaxHeight() const {
  if (!GetNative())
    return 0;
  auto* scroll = static_cast<views::ScrollView*>(GetNative());
  return scroll->GetMaxHeight();
}

gfx::Rect NativeScrollView::GetVisibleRect() const {
  if (!GetNative())
    return gfx::Rect();
  auto* scroll = static_cast<views::ScrollView*>(GetNative());
  return scroll->GetVisibleRect();
}

void NativeScrollView::SetAllowKeyboardScrolling(bool allow) {
  if (!GetNative())
    return;
  auto* scroll = static_cast<views::ScrollView*>(GetNative());
  scroll->SetAllowKeyboardScrolling(allow);
}

bool NativeScrollView::GetAllowKeyboardScrolling() const {
  if (!GetNative())
    return false;
  auto* scroll = static_cast<views::ScrollView*>(GetNative());
  return scroll->GetAllowKeyboardScrolling();
}

void NativeScrollView::SetDrawOverflowIndicator(bool indicator) {
  if (!GetNative())
    return;
  auto* scroll = static_cast<views::ScrollView*>(GetNative());
  scroll->SetDrawOverflowIndicator(indicator);
}

bool NativeScrollView::GetDrawOverflowIndicator() const {
  if (!GetNative())
    return false;
  auto* scroll = static_cast<views::ScrollView*>(GetNative());
  return scroll->GetDrawOverflowIndicator();
}

}  // namespace electron
