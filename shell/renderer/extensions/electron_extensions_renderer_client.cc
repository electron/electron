// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "shell/renderer/extensions/electron_extensions_renderer_client.h"

#include "content/public/renderer/render_thread.h"
#include "extensions/renderer/dispatcher.h"
#include "shell/common/world_ids.h"
#include "shell/renderer/extensions/electron_extensions_dispatcher_delegate.h"

namespace electron {

ElectronExtensionsRendererClient::ElectronExtensionsRendererClient()
    : dispatcher_(std::make_unique<extensions::Dispatcher>(
          std::make_unique<ElectronExtensionsDispatcherDelegate>())) {
  dispatcher_->OnRenderThreadStarted(content::RenderThread::Get());
}

ElectronExtensionsRendererClient::~ElectronExtensionsRendererClient() {}

bool ElectronExtensionsRendererClient::IsIncognitoProcess() const {
  // app_shell doesn't support off-the-record contexts.
  return false;
}

int ElectronExtensionsRendererClient::GetLowestIsolatedWorldId() const {
  return WorldIDs::ISOLATED_WORLD_ID_EXTENSIONS;
}

extensions::Dispatcher* ElectronExtensionsRendererClient::GetDispatcher() {
  return dispatcher_.get();
}

bool ElectronExtensionsRendererClient::
    ExtensionAPIEnabledForServiceWorkerScript(const GURL& scope,
                                              const GURL& script_url) const {
  // TODO(nornagon): adapt logic from chrome's version
  return true;
}

bool ElectronExtensionsRendererClient::AllowPopup() {
  // TODO(samuelmaddock):
  return false;
}

void ElectronExtensionsRendererClient::RunScriptsAtDocumentStart(
    content::RenderFrame* render_frame) {
  dispatcher_->RunScriptsAtDocumentStart(render_frame);
}

void ElectronExtensionsRendererClient::RunScriptsAtDocumentEnd(
    content::RenderFrame* render_frame) {
  dispatcher_->RunScriptsAtDocumentEnd(render_frame);
}

void ElectronExtensionsRendererClient::RunScriptsAtDocumentIdle(
    content::RenderFrame* render_frame) {
  dispatcher_->RunScriptsAtDocumentIdle(render_frame);
}

}  // namespace electron
