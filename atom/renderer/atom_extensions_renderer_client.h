// Copyright (c) 2017 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef EXTENSIONS_ATOM_RENDERER_ATOM_EXTENSIONS_RENDERER_CLIENT_H_
#define EXTENSIONS_ATOM_RENDERER_ATOM_EXTENSIONS_RENDERER_CLIENT_H_

#include <memory>

#include "base/macros.h"
#include "extensions/renderer/extensions_renderer_client.h"

namespace content {
class RenderFrame;
}

namespace extensions {
class Dispatcher;
}

namespace atom {

class AtomExtensionsRendererClient
    : public extensions::ExtensionsRendererClient {
 public:
  AtomExtensionsRendererClient();
  ~AtomExtensionsRendererClient() override;

  // ExtensionsRendererClient implementation.
  bool IsIncognitoProcess() const override;
  int GetLowestIsolatedWorldId() const override;
  extensions::Dispatcher* GetDispatcher() override;

  bool AllowPopup();

  void RunScriptsAtDocumentStart(content::RenderFrame* render_frame);
  void RunScriptsAtDocumentEnd(content::RenderFrame* render_frame);
  void RunScriptsAtDocumentIdle(content::RenderFrame* render_frame);

 private:
  std::unique_ptr<extensions::Dispatcher> dispatcher_;

  DISALLOW_COPY_AND_ASSIGN(AtomExtensionsRendererClient);
};

}  // namespace atom

#endif  // EXTENSIONS_ATOM_RENDERER_ATOM_EXTENSIONS_RENDERER_CLIENT_H_
