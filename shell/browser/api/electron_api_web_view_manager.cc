// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "content/public/browser/browser_context.h"
#include "shell/browser/web_contents_preferences.h"
#include "shell/browser/web_contents_zoom_controller.h"
#include "shell/browser/web_view_manager.h"
#include "shell/common/gin_converters/content_converter.h"
#include "shell/common/gin_helper/dictionary.h"
#include "shell/common/node_includes.h"
#include "shell/common/options_switches.h"

using electron::WebContentsPreferences;

namespace {

void AddGuest(int guest_instance_id,
              content::WebContents* embedder,
              content::WebContents* guest_web_contents,
              const gin_helper::Dictionary& options) {
  auto* manager = electron::WebViewManager::GetWebViewManager(embedder);
  if (manager)
    manager->AddGuest(guest_instance_id, embedder, guest_web_contents);

  double zoom_factor;
  if (options.Get(electron::options::kZoomFactor, &zoom_factor)) {
    electron::WebContentsZoomController::FromWebContents(guest_web_contents)
        ->SetDefaultZoomFactor(zoom_factor);
  }
}

void RemoveGuest(content::WebContents* embedder, int guest_instance_id) {
  auto* manager = electron::WebViewManager::GetWebViewManager(embedder);
  if (manager)
    manager->RemoveGuest(guest_instance_id);
}

void Initialize(v8::Local<v8::Object> exports,
                v8::Local<v8::Value> unused,
                v8::Local<v8::Context> context,
                void* priv) {
  gin_helper::Dictionary dict(context->GetIsolate(), exports);
  dict.SetMethod("addGuest", &AddGuest);
  dict.SetMethod("removeGuest", &RemoveGuest);
}

}  // namespace

NODE_LINKED_BINDING_CONTEXT_AWARE(electron_browser_web_view_manager, Initialize)
