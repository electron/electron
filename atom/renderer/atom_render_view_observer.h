// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_RENDERER_ATOM_RENDER_VIEW_OBSERVER_H_
#define ATOM_RENDERER_ATOM_RENDER_VIEW_OBSERVER_H_

#include "content/public/renderer/render_view_observer.h"

namespace atom {

class AtomRenderViewObserver : public content::RenderViewObserver {
 public:
  explicit AtomRenderViewObserver(content::RenderView* render_view);

 protected:
  ~AtomRenderViewObserver() override;

 private:
  // content::RenderViewObserver implementation.
  bool OnMessageReceived(const IPC::Message& message) override;
  void OnDestruct() override;

  void OnOffscreen();

  DISALLOW_COPY_AND_ASSIGN(AtomRenderViewObserver);
};

}  // namespace atom

#endif  // ATOM_RENDERER_ATOM_RENDER_VIEW_OBSERVER_H_
