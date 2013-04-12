// Copyright (c) 2013 GitHub, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "renderer/atom_render_view_observer.h"

#include "third_party/WebKit/Source/WebKit/chromium/public/WebDocument.h"
#include "third_party/WebKit/Source/WebKit/chromium/public/WebFrame.h"
#include "v8/include/v8.h"

namespace atom {

AtomRenderViewObserver::AtomRenderViewObserver(
    content::RenderView *render_view)
    : content::RenderViewObserver(render_view) {
}

AtomRenderViewObserver::~AtomRenderViewObserver() {
}

void AtomRenderViewObserver::DidClearWindowObject(WebKit::WebFrame* frame) {
}

}  // namespace atom
