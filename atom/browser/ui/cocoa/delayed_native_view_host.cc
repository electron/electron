// Copyright (c) 2018 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/ui/cocoa/delayed_native_view_host.h"

namespace atom {

DelayedNativeViewHost::DelayedNativeViewHost(gfx::NativeView native_view)
    : native_view_(native_view) {}

DelayedNativeViewHost::~DelayedNativeViewHost() {}

void DelayedNativeViewHost::ViewHierarchyChanged(
    const ViewHierarchyChangedDetails& details) {
  NativeViewHost::ViewHierarchyChanged(details);
  if (details.is_add && GetWidget())
    Attach(native_view_);
}

}  // namespace atom
