// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "shell/renderer/extensions/atom_extensions_renderer_client.h"

#include "content/public/renderer/render_thread.h"
#include "extensions/renderer/dispatcher.h"
#include "shell/renderer/extensions/electron_extensions_dispatcher_delegate.h"

namespace electron {

AtomExtensionsRendererClient::AtomExtensionsRendererClient()
    : dispatcher_(std::make_unique<extensions::Dispatcher>(
          std::make_unique<ElectronExtensionsDispatcherDelegate>())) {
  dispatcher_->OnRenderThreadStarted(content::RenderThread::Get());
}

AtomExtensionsRendererClient::~AtomExtensionsRendererClient() {}

bool AtomExtensionsRendererClient::IsIncognitoProcess() const {
  // app_shell doesn't support off-the-record contexts.
  return false;
}

int AtomExtensionsRendererClient::GetLowestIsolatedWorldId() const {
  // app_shell doesn't need to reserve world IDs for anything other than
  // extensions, so we always return 1. Note that 0 is reserved for the global
  // world.
  // TODO(samuelmaddock): skip electron worlds
  return 10;
}

extensions::Dispatcher* AtomExtensionsRendererClient::GetDispatcher() {
  return dispatcher_.get();
}

bool AtomExtensionsRendererClient::ExtensionAPIEnabledForServiceWorkerScript(
    const GURL& scope,
    const GURL& script_url) const {
  // TODO(nornagon): adapt logic from chrome's version
  return true;
}

bool AtomExtensionsRendererClient::AllowPopup() {
  // TODO(samuelmaddock):
  return false;
}

void AtomExtensionsRendererClient::RunScriptsAtDocumentStart(
    content::RenderFrame* render_frame) {
  dispatcher_->RunScriptsAtDocumentStart(render_frame);
}

void AtomExtensionsRendererClient::RunScriptsAtDocumentEnd(
    content::RenderFrame* render_frame) {
  dispatcher_->RunScriptsAtDocumentEnd(render_frame);
}

void AtomExtensionsRendererClient::RunScriptsAtDocumentIdle(
    content::RenderFrame* render_frame) {
  dispatcher_->RunScriptsAtDocumentIdle(render_frame);
}

}  // namespace electron
