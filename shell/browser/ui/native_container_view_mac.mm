// Copyright (c) 2022 GitHub, Inc.

#include "shell/browser/ui/native_container_view.h"

#include "shell/browser/ui/cocoa/electron_native_view.h"

namespace electron {

void NativeContainerView::InitContainerView() {
  SetNativeView([[ElectronNativeView alloc] init]);
}

void NativeContainerView::AddChildViewImpl(NativeView* view) {
  [GetNative() addSubview:view->GetNative()];
  NativeViewPrivate* priv = [GetNative() nativeViewPrivate];
  if (priv->wants_layer_infected) {
    [view->GetNative() setWantsLayer:YES];
  } else {
    if (IsNativeView(view->GetNative()) &&
        [view->GetNative() nativeViewPrivate]->wants_layer_infected) {
      priv->wants_layer_infected = true;
      SetWantsLayer(true);
      for (int i = 0; i < ChildCount(); ++i)
        [children_[i]->GetNative() setWantsLayer:YES];
    }
  }
}

void NativeContainerView::RemoveChildViewImpl(NativeView* view) {
  [view->GetNative() removeFromSuperview];
  NSView* native_view = view->GetNative();
  if (IsNativeView(native_view))
    [native_view setWantsLayer:[native_view nativeViewPrivate]->wants_layer];
  else
    [native_view setWantsLayer:NO];
}

void NativeContainerView::UpdateDraggableRegions() {
  for (auto view : children_)
    view->UpdateDraggableRegions();
}

}  // namespace electron
