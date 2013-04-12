// Copyright (c) 2013 GitHub, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ATOM_RENDERER_ATOM_RENDER_VIEW_OBSERVER_
#define ATOM_RENDERER_ATOM_RENDER_VIEW_OBSERVER_

#include "content/public/renderer/render_view_observer.h"

namespace atom {

class AtomRenderViewObserver : content::RenderViewObserver {
public:
  explicit AtomRenderViewObserver(content::RenderView*);

private:
  ~AtomRenderViewObserver();

  virtual void DidClearWindowObject(WebKit::WebFrame*) OVERRIDE;

  DISALLOW_COPY_AND_ASSIGN(AtomRenderViewObserver);
};

}  // namespace atom

#endif  // ATOM_RENDERER_ATOM_RENDER_VIEW_OBSERVER_
