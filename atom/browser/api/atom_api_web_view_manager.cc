// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/web_contents_preferences.h"
#include "atom/browser/web_contents_zoom_controller.h"
#include "atom/browser/web_view_manager.h"
#include "atom/common/native_mate_converters/content_converter.h"
#include "atom/common/native_mate_converters/value_converter.h"
#include "atom/common/options_switches.h"
#include "content/public/browser/browser_context.h"
#include "native_mate/dictionary.h"

// Must be the last in the includes list.
// See https://github.com/electron/electron/issues/10363
#include "atom/common/node_includes.h"

using atom::WebContentsPreferences;

namespace {

void AddGuest(int guest_instance_id,
              int element_instance_id,
              content::WebContents* embedder,
              content::WebContents* guest_web_contents,
              const base::DictionaryValue& options) {
  auto* manager = atom::WebViewManager::GetWebViewManager(embedder);
  if (manager)
    manager->AddGuest(guest_instance_id, element_instance_id, embedder,
                      guest_web_contents);

  double zoom_factor;
  if (options.GetDouble(atom::options::kZoomFactor, &zoom_factor)) {
    atom::WebContentsZoomController::FromWebContents(guest_web_contents)
        ->SetDefaultZoomFactor(zoom_factor);
  }

  WebContentsPreferences::From(guest_web_contents)->Merge(options);
}

void RemoveGuest(content::WebContents* embedder, int guest_instance_id) {
  auto* manager = atom::WebViewManager::GetWebViewManager(embedder);
  if (manager)
    manager->RemoveGuest(guest_instance_id);
}

void Initialize(v8::Local<v8::Object> exports,
                v8::Local<v8::Value> unused,
                v8::Local<v8::Context> context,
                void* priv) {
  mate::Dictionary dict(context->GetIsolate(), exports);
  dict.SetMethod("addGuest", &AddGuest);
  dict.SetMethod("removeGuest", &RemoveGuest);
}

}  // namespace

NODE_BUILTIN_MODULE_CONTEXT_AWARE(atom_browser_web_view_manager, Initialize)
