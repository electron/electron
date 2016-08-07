// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#pragma once

#include "base/strings/string16.h"
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
  void DidCreateDocumentElement(blink::WebLocalFrame* frame) override;
  void DraggableRegionsChanged(blink::WebFrame* frame) override;
  bool OnMessageReceived(const IPC::Message& message) override;

  void OnBrowserMessage(bool send_to_all,
                        const base::string16& channel,
                        const base::ListValue& args);

  // Weak reference to renderer client.
  AtomRendererClient* renderer_client_;

  // Whether the document object has been created.
  bool document_created_;

  DISALLOW_COPY_AND_ASSIGN(AtomRenderViewObserver);
};

}  // namespace atom
