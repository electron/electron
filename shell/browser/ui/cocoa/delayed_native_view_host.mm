// Copyright (c) 2018 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/ui/cocoa/delayed_native_view_host.h"
#include "base/apple/owned_objc.h"
#include "shell/browser/ui/cocoa/electron_inspectable_web_contents_view.h"

namespace electron {

DelayedNativeViewHost::DelayedNativeViewHost(gfx::NativeView native_view)
    : native_view_(native_view) {}

DelayedNativeViewHost::~DelayedNativeViewHost() = default;

void DelayedNativeViewHost::ViewHierarchyChanged(
    const views::ViewHierarchyChangedDetails& details) {
  // NativeViewHost doesn't expect to have children, so filter the
  // ViewHierarchyChanged events before passing them on.
  if (details.child == this) {
    NativeViewHost::ViewHierarchyChanged(details);
    if (details.is_add && GetWidget() && !native_view())
      Attach(native_view_);
  }
}

bool DelayedNativeViewHost::OnMousePressed(const ui::MouseEvent& ui_event) {
  // NativeViewHost::OnMousePressed normally isn't called, but
  // NativeWidgetMacNSWindow specifically carves out an event here for
  // right-mouse-button clicks. We want to forward them to the web content, so
  // handle them here.
  // See:
  // https://source.chromium.org/chromium/chromium/src/+/main:components/remote_cocoa/app_shim/native_widget_mac_nswindow.mm;l=415-421;drc=a5af91924bafb85426e091c6035801990a6dc697

  ElectronInspectableWebContentsView* inspectable_web_contents_view =
      (ElectronInspectableWebContentsView*)native_view_.GetNativeNSView();
  [inspectable_web_contents_view
      redispatchContextMenuEvent:base::apple::OwnedNSEvent(
                                     ui_event.native_event())];

  return true;
}

}  // namespace electron
