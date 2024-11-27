// Copyright (c) 2017 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_RENDERER_EXTENSIONS_ELECTRON_EXTENSIONS_RENDERER_CLIENT_H_
#define ELECTRON_SHELL_RENDERER_EXTENSIONS_ELECTRON_EXTENSIONS_RENDERER_CLIENT_H_

#include <memory>

#include "extensions/renderer/extensions_renderer_client.h"
#include "v8/include/v8-local-handle.h"

class GURL;

namespace blink {
class WebElement;
class WebFrame;
class WebURL;
class WebView;
}  // namespace blink

namespace content {
class RenderFrame;
struct WebPluginInfo;
}  // namespace content

namespace extensions {
class Dispatcher;
}

namespace v8 {
class Isolate;
class Object;
}  // namespace v8

namespace electron {

class ElectronExtensionsRendererClient
    : public extensions::ExtensionsRendererClient {
 public:
  ElectronExtensionsRendererClient();
  ~ElectronExtensionsRendererClient() override;

  // disable copy
  ElectronExtensionsRendererClient(const ElectronExtensionsRendererClient&) =
      delete;
  ElectronExtensionsRendererClient& operator=(
      const ElectronExtensionsRendererClient&) = delete;

  // Get the LazyInstance for ElectronExtensionsRendererClient.
  static ElectronExtensionsRendererClient* GetInstance();

  // ExtensionsRendererClient implementation.
  bool IsIncognitoProcess() const override;
  int GetLowestIsolatedWorldId() const override;

  bool AllowPopup();

  static bool MaybeCreateMimeHandlerView(
      const blink::WebElement& plugin_element,
      const GURL& resource_url,
      const std::string& mime_type,
      const content::WebPluginInfo& plugin_info);
  v8::Local<v8::Object> GetScriptableObject(
      const blink::WebElement& plugin_element,
      v8::Isolate* isolate);

  void RunScriptsAtDocumentStart(content::RenderFrame* render_frame);
  void RunScriptsAtDocumentEnd(content::RenderFrame* render_frame);
  void RunScriptsAtDocumentIdle(content::RenderFrame* render_frame);
};

}  // namespace electron

#endif  // ELECTRON_SHELL_RENDERER_EXTENSIONS_ELECTRON_EXTENSIONS_RENDERER_CLIENT_H_
