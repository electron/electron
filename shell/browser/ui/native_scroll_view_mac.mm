// Copyright (c) 2022 GitHub, Inc.

#include "shell/browser/ui/native_scroll_view.h"

#include "shell/browser/ui/cocoa/electron_native_view.h"

@interface ElectronNativeScrollView
    : NSScrollView <ElectronNativeViewProtocol> {
 @private
  electron::NativeViewPrivate private_;
  electron::NativeScrollView* shell_;
}
- (id)initWithShell:(electron::NativeScrollView*)shell;
@end

@implementation ElectronNativeScrollView

- (id)initWithShell:(electron::NativeScrollView*)shell {
  if ((self = [super init])) {
    shell_ = shell;
  }
  return self;
}

- (void)dealloc {
  if ([self shell])
    [self shell]->NotifyViewIsDeleting();
  [super dealloc];
}

- (electron::NativeViewPrivate*)nativeViewPrivate {
  return &private_;
}

- (void)setNativeBackgroundColor:(SkColor)color {
}

@end

namespace electron {

void NativeScrollView::InitScrollView() {
  auto* scroll = [[ElectronNativeScrollView alloc] initWithShell:this];
  scroll.drawsBackground = NO;
  if (scroll.scrollerStyle == NSScrollerStyleOverlay) {
    scroll.hasHorizontalScroller = YES;
    scroll.hasVerticalScroller = YES;
  }
  [scroll.contentView setCopiesOnScroll:NO];
  SetNativeView(scroll);
}

void NativeScrollView::SetContentViewImpl(NativeView* view) {
  auto* scroll = static_cast<NSScrollView*>(GetNative());
  scroll.documentView = view->GetNative();
}

void NativeScrollView::DetachChildViewImpl() {
  auto* scroll = static_cast<NSScrollView*>(GetNative());
  scroll.documentView = nil;
}

void NativeScrollView::SetHorizontalScrollBarMode(ScrollBarMode mode) {
  auto* scroll = static_cast<ElectronNativeScrollView*>(GetNative());
  scroll.hasHorizontalScroller = mode != ScrollBarMode::kDisabled;
}

ScrollBarMode NativeScrollView::GetHorizontalScrollBarMode() const {
  auto* scroll = static_cast<ElectronNativeScrollView*>(GetNative());
  return scroll.hasHorizontalScroller ? ScrollBarMode::kEnabled
                                      : ScrollBarMode::kDisabled;
}

void NativeScrollView::SetVerticalScrollBarMode(ScrollBarMode mode) {
  auto* scroll = static_cast<ElectronNativeScrollView*>(GetNative());
  scroll.hasVerticalScroller = mode != ScrollBarMode::kDisabled;
}

ScrollBarMode NativeScrollView::GetVerticalScrollBarMode() const {
  auto* scroll = static_cast<ElectronNativeScrollView*>(GetNative());
  return scroll.hasVerticalScroller ? ScrollBarMode::kEnabled
                                    : ScrollBarMode::kDisabled;
}

void NativeScrollView::SetHorizontalScrollElasticity(
    ScrollElasticity elasticity) {
  auto* scroll = static_cast<ElectronNativeScrollView*>(GetNative());
  scroll.horizontalScrollElasticity =
      static_cast<NSScrollElasticity>(elasticity);
}

ScrollElasticity NativeScrollView::GetHorizontalScrollElasticity() const {
  auto* scroll = static_cast<ElectronNativeScrollView*>(GetNative());
  return static_cast<ScrollElasticity>(scroll.horizontalScrollElasticity);
}

void NativeScrollView::SetVerticalScrollElasticity(
    ScrollElasticity elasticity) {
  auto* scroll = static_cast<ElectronNativeScrollView*>(GetNative());
  scroll.verticalScrollElasticity = static_cast<NSScrollElasticity>(elasticity);
}

ScrollElasticity NativeScrollView::GetVerticalScrollElasticity() const {
  auto* scroll = static_cast<ElectronNativeScrollView*>(GetNative());
  return static_cast<ScrollElasticity>(scroll.verticalScrollElasticity);
}

void NativeScrollView::UpdateDraggableRegions() {
  if (content_view_.get())
    content_view_->UpdateDraggableRegions();
}

}  // namespace electron
