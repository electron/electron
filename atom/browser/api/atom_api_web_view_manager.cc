// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/api/atom_api_web_contents.h"
#include "atom/browser/web_contents_preferences.h"
#include "atom/browser/web_view_manager.h"
#include "atom/common/native_mate_converters/value_converter.h"
#include "atom/common/node_includes.h"
#include "content/public/browser/browser_context.h"
#include "native_mate/dictionary.h"

using atom::WebContentsPreferences;

namespace mate {

template<>
struct Converter<content::WebContents*> {
  static bool FromV8(v8::Isolate* isolate, v8::Local<v8::Value> val,
                     content::WebContents** out) {
    atom::api::WebContents* contents;
    if (!Converter<atom::api::WebContents*>::FromV8(isolate, val, &contents))
      return false;
    *out = contents->web_contents();
    return true;
  }
};

}  // namespace mate

namespace {

atom::WebViewManager* GetWebViewManager(content::WebContents* web_contents) {
  auto context = web_contents->GetBrowserContext();
  if (context) {
    auto manager = context->GetGuestManager();
    return static_cast<atom::WebViewManager*>(manager);
  } else {
    return nullptr;
  }
}

void AddGuest(int guest_instance_id,
              int element_instance_id,
              content::WebContents* embedder,
              content::WebContents* guest_web_contents,
              const base::DictionaryValue& options) {
  auto manager = GetWebViewManager(embedder);
  if (manager)
    manager->AddGuest(guest_instance_id, element_instance_id, embedder,
                      guest_web_contents);

  WebContentsPreferences::FromWebContents(guest_web_contents)->Merge(options);
}

void RemoveGuest(content::WebContents* embedder, int guest_instance_id) {
  auto manager = GetWebViewManager(embedder);
  if (manager)
    manager->RemoveGuest(guest_instance_id);
}

void Initialize(v8::Local<v8::Object> exports, v8::Local<v8::Value> unused,
                v8::Local<v8::Context> context, void* priv) {
  mate::Dictionary dict(context->GetIsolate(), exports);
  dict.SetMethod("addGuest", &AddGuest);
  dict.SetMethod("removeGuest", &RemoveGuest);
}

}  // namespace

NODE_MODULE_CONTEXT_AWARE_BUILTIN(atom_browser_web_view_manager, Initialize)
