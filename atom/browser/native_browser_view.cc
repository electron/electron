// Copyright (c) 2017 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include <vector>

#include "atom/browser/native_browser_view.h"

#include "atom/browser/api/atom_api_web_contents.h"
#include "brightray/browser/inspectable_web_contents_view.h"

namespace atom {

NativeBrowserView::NativeBrowserView(
    brightray::InspectableWebContentsView* web_contents_view)
    : web_contents_view_(web_contents_view) {}

NativeBrowserView::~NativeBrowserView() {}

}  // namespace atom
