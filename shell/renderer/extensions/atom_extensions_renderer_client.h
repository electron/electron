// Copyright (c) 2017 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef SHELL_RENDERER_EXTENSIONS_ATOM_EXTENSIONS_RENDERER_CLIENT_H_
#define SHELL_RENDERER_EXTENSIONS_ATOM_EXTENSIONS_RENDERER_CLIENT_H_

#include <memory>

#include "base/macros.h"
#include "content/public/common/content_constants.h"
#include "content/public/common/webplugininfo.h"
#include "content/public/renderer/browser_plugin_delegate.h"
#include "extensions/renderer/extensions_renderer_client.h"
#include "extensions/renderer/guest_view/extensions_guest_view_container.h"
#include "extensions/renderer/guest_view/mime_handler_view/mime_handler_view_container.h"

namespace content {
class RenderFrame;
}

namespace extensions {
class Dispatcher;
}

namespace electron {

class AtomExtensionsRendererClient
    : public extensions::ExtensionsRendererClient {
 public:
  AtomExtensionsRendererClient();
  ~AtomExtensionsRendererClient() override;

  // ExtensionsRendererClient implementation.
  bool IsIncognitoProcess() const override;
  int GetLowestIsolatedWorldId() const override;
  extensions::Dispatcher* GetDispatcher() override;
  bool ExtensionAPIEnabledForServiceWorkerScript(
      const GURL& scope,
      const GURL& script_url) const override;

  bool AllowPopup();

  void RunScriptsAtDocumentStart(content::RenderFrame* render_frame);
  void RunScriptsAtDocumentEnd(content::RenderFrame* render_frame);
  void RunScriptsAtDocumentIdle(content::RenderFrame* render_frame);

  static content::BrowserPluginDelegate* CreateBrowserPluginDelegate(
      content::RenderFrame* render_frame,
      const content::WebPluginInfo& info,
      const std::string& mime_type,
      const GURL& original_url);

 private:
  std::unique_ptr<extensions::Dispatcher> dispatcher_;

  DISALLOW_COPY_AND_ASSIGN(AtomExtensionsRendererClient);
};

}  // namespace electron

#endif  // SHELL_RENDERER_EXTENSIONS_ATOM_EXTENSIONS_RENDERER_CLIENT_H_
