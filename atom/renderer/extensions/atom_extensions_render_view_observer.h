// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_RENDERER_EXTENSIONS_ATOM_EXTENSIONS_RENDER_VIEW_OBSERVER_H_
#define ATOM_RENDERER_EXTENSIONS_ATOM_EXTENSIONS_RENDER_VIEW_OBSERVER_H_

#include "base/strings/string16.h"
#include "content/public/renderer/render_view_observer.h"

namespace base {
class ListValue;
}

namespace extensions {

class AtomExtensionsRenderViewObserver : public content::RenderViewObserver {
 public:
  explicit AtomExtensionsRenderViewObserver(content::RenderView* render_view);

 protected:
  virtual ~AtomExtensionsRenderViewObserver();

 private:
  // content::RenderViewObserver implementation.
  void DidCreateDocumentElement(blink::WebLocalFrame* frame) override;
  bool OnMessageReceived(const IPC::Message& message) override;
  void OnBrowserMessage(const base::string16& channel,
                        const base::ListValue& args);

  DISALLOW_COPY_AND_ASSIGN(AtomExtensionsRenderViewObserver);
};

}  // namespace extensions

#endif  // ATOM_RENDERER_EXTENSIONS_ATOM_EXTENSIONS_RENDER_VIEW_OBSERVER_H_
