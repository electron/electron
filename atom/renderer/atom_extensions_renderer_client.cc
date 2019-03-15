// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "extensions/shell/renderer/shell_extensions_renderer_client.h"

#include "content/public/renderer/render_thread.h"
#include "extensions/renderer/dispatcher.h"
#include "extensions/renderer/dispatcher_delegate.h"

namespace atom {

AtomExtensionsRendererClient::AtomExtensionsRendererClient()
    : dispatcher_(std::make_unique<extensions::Dispatcher>(
          std::make_unique<extensions::DispatcherDelegate>())) {
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

Dispatcher* AtomExtensionsRendererClient::GetDispatcher() {
  return dispatcher_.get();
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

}  // namespace atom
