// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_RENDERER_ELECTRON_RENDER_VIEW_OBSERVER_H_
#define ELECTRON_RENDERER_ELECTRON_RENDER_VIEW_OBSERVER_H_

#include "base/strings/string16.h"
#include "content/public/renderer/render_view_observer.h"

namespace base {
class ListValue;
}

namespace electron {

class ElectronRendererClient;

class ElectronRenderViewObserver : public content::RenderViewObserver {
 public:
  explicit ElectronRenderViewObserver(content::RenderView* render_view,
                                  ElectronRendererClient* renderer_client);

 protected:
  virtual ~ElectronRenderViewObserver();

 private:
  // content::RenderViewObserver implementation.
  void DidCreateDocumentElement(blink::WebLocalFrame* frame) override;
  void DraggableRegionsChanged(blink::WebFrame* frame) override;
  bool OnMessageReceived(const IPC::Message& message) override;

  void OnBrowserMessage(const base::string16& channel,
                        const base::ListValue& args);

  // Weak reference to renderer client.
  ElectronRendererClient* renderer_client_;

  // Whether the document object has been created.
  bool document_created_;

  DISALLOW_COPY_AND_ASSIGN(ElectronRenderViewObserver);
};

}  // namespace electron

#endif  // ELECTRON_RENDERER_ELECTRON_RENDER_VIEW_OBSERVER_H_
