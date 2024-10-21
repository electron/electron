// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "shell/renderer/extensions/electron_extensions_renderer_client.h"

#include <string>

#include "base/lazy_instance.h"
#include "content/public/renderer/render_frame.h"
#include "content/public/renderer/render_thread.h"
#include "extensions/common/constants.h"
#include "extensions/common/manifest_handlers/background_info.h"
#include "extensions/renderer/dispatcher.h"
#include "extensions/renderer/guest_view/mime_handler_view/mime_handler_view_container_manager.h"
#include "shell/common/world_ids.h"
#include "third_party/blink/public/platform/web_url.h"
#include "third_party/blink/public/web/web_document.h"
#include "third_party/blink/public/web/web_local_frame.h"
#include "third_party/blink/public/web/web_plugin_params.h"

namespace electron {

ElectronExtensionsRendererClient::ElectronExtensionsRendererClient() {}

// static
ElectronExtensionsRendererClient*
ElectronExtensionsRendererClient::GetInstance() {
  static base::LazyInstance<ElectronExtensionsRendererClient>::Leaky client =
      LAZY_INSTANCE_INITIALIZER;
  return client.Pointer();
}

ElectronExtensionsRendererClient::~ElectronExtensionsRendererClient() = default;

bool ElectronExtensionsRendererClient::IsIncognitoProcess() const {
  // app_shell doesn't support off-the-record contexts.
  return false;
}

int ElectronExtensionsRendererClient::GetLowestIsolatedWorldId() const {
  return WorldIDs::ISOLATED_WORLD_ID_EXTENSIONS;
}

// static
bool ElectronExtensionsRendererClient::MaybeCreateMimeHandlerView(
    const blink::WebElement& plugin_element,
    const GURL& resource_url,
    const std::string& mime_type,
    const content::WebPluginInfo& plugin_info) {
  return extensions::MimeHandlerViewContainerManager::Get(
             content::RenderFrame::FromWebFrame(
                 plugin_element.GetDocument().GetFrame()),
             true /* create_if_does_not_exist */)
      ->CreateFrameContainer(plugin_element, resource_url, mime_type,
                             plugin_info);
}

v8::Local<v8::Object> ElectronExtensionsRendererClient::GetScriptableObject(
    const blink::WebElement& plugin_element,
    v8::Isolate* isolate) {
  // If there is a MimeHandlerView that can provide the scriptable object then
  // MaybeCreateMimeHandlerView must have been called before and a container
  // manager should exist.
  auto* container_manager = extensions::MimeHandlerViewContainerManager::Get(
      content::RenderFrame::FromWebFrame(
          plugin_element.GetDocument().GetFrame()),
      false /* create_if_does_not_exist */);
  if (container_manager)
    return container_manager->GetScriptableObject(plugin_element, isolate);
  return v8::Local<v8::Object>();
}

bool ElectronExtensionsRendererClient::AllowPopup() {
  // TODO(samuelmaddock): what do we want to allow here?
  return false;
}

void ElectronExtensionsRendererClient::RunScriptsAtDocumentStart(
    content::RenderFrame* render_frame) {
  dispatcher()->RunScriptsAtDocumentStart(render_frame);
}

void ElectronExtensionsRendererClient::RunScriptsAtDocumentEnd(
    content::RenderFrame* render_frame) {
  dispatcher()->RunScriptsAtDocumentEnd(render_frame);
}

void ElectronExtensionsRendererClient::RunScriptsAtDocumentIdle(
    content::RenderFrame* render_frame) {
  dispatcher()->RunScriptsAtDocumentIdle(render_frame);
}

}  // namespace electron
