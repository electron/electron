// Copyright (c) 2016 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/common/api/remote/remote_callback_freer.h"

#include "base/strings/utf_string_conversions.h"
#include "base/values.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_contents.h"
#include "electron/shell/common/api/api.mojom.h"
#include "mojo/public/cpp/bindings/associated_remote.h"
#include "third_party/blink/public/common/associated_interfaces/associated_interface_provider.h"

namespace electron {

// static
void RemoteCallbackFreer::BindTo(v8::Isolate* isolate,
                                 v8::Local<v8::Object> target,
                                 int process_id,
                                 int frame_id,
                                 const std::string& context_id,
                                 int object_id,
                                 content::WebContents* web_contents) {
  new RemoteCallbackFreer(isolate, target, process_id, frame_id, context_id,
                          object_id, web_contents);
}

RemoteCallbackFreer::RemoteCallbackFreer(v8::Isolate* isolate,
                                         v8::Local<v8::Object> target,
                                         int process_id,
                                         int frame_id,
                                         const std::string& context_id,
                                         int object_id,
                                         content::WebContents* web_contents)
    : ObjectLifeMonitor(isolate, target),
      content::WebContentsObserver(web_contents),
      process_id_(process_id),
      frame_id_(frame_id),
      context_id_(context_id),
      object_id_(object_id) {}

RemoteCallbackFreer::~RemoteCallbackFreer() = default;

void RemoteCallbackFreer::RunDestructor() {
  auto* rfh = content::RenderFrameHost::FromID(process_id_, frame_id_);
  if (!rfh || !rfh->IsRenderFrameLive() ||
      content::WebContents::FromRenderFrameHost(rfh) != web_contents())
    return;

  mojo::AssociatedRemote<mojom::ElectronRenderer> electron_renderer;
  rfh->GetRemoteAssociatedInterfaces()->GetInterface(&electron_renderer);
  electron_renderer->DereferenceRemoteJSCallback(context_id_, object_id_);

  Observe(nullptr);
}

void RemoteCallbackFreer::RenderViewDeleted(content::RenderViewHost*) {
  delete this;
}

}  // namespace electron
