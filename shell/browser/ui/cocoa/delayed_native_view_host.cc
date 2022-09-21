// Copyright (c) 2018 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/ui/cocoa/delayed_native_view_host.h"

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

}  // namespace electron
