// Copyright (c) 2013 GitHub, Inc. All rights reserved.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_RENDERER_ATOM_RENDER_VIEW_OBSERVER_H_
#define ATOM_RENDERER_ATOM_RENDER_VIEW_OBSERVER_H_

#include "content/public/renderer/render_view_observer.h"

namespace base {
class ListValue;
}

namespace atom {

class AtomRendererClient;

class AtomRenderViewObserver : public content::RenderViewObserver {
 public:
  explicit AtomRenderViewObserver(content::RenderView* render_view,
                                  AtomRendererClient* renderer_client);

 protected:
  virtual ~AtomRenderViewObserver();

 private:
  // content::RenderViewObserver implementation.
  virtual void DraggableRegionsChanged(WebKit::WebFrame* frame) OVERRIDE;
  virtual bool OnMessageReceived(const IPC::Message& message) OVERRIDE;

  void OnBrowserMessage(const string16& channel,
                        const base::ListValue& args);

  // Weak reference to renderer client.
  AtomRendererClient* renderer_client_;

  DISALLOW_COPY_AND_ASSIGN(AtomRenderViewObserver);
};

}  // namespace atom

#endif  // ATOM_RENDERER_ATOM_RENDER_VIEW_OBSERVER_H_
