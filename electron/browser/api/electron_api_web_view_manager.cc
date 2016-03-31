// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "electron/browser/api/electron_api_web_contents.h"
#include "electron/browser/web_contents_preferences.h"
#include "electron/browser/web_view_manager.h"
#include "electron/common/native_mate_converters/value_converter.h"
#include "electron/common/node_includes.h"
#include "content/public/browser/browser_context.h"
#include "native_mate/dictionary.h"

using electron::WebContentsPreferences;

namespace mate {

template<>
struct Converter<content::WebContents*> {
  static bool FromV8(v8::Isolate* isolate, v8::Local<v8::Value> val,
                     content::WebContents** out) {
    electron::api::WebContents* contents;
    if (!Converter<electron::api::WebContents*>::FromV8(isolate, val, &contents))
      return false;
    *out = contents->web_contents();
    return true;
  }
};

}  // namespace mate

namespace {

electron::WebViewManager* GetWebViewManager(content::WebContents* web_contents) {
  auto context = web_contents->GetBrowserContext();
  if (context) {
    auto manager = context->GetGuestManager();
    return static_cast<electron::WebViewManager*>(manager);
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

NODE_MODULE_CONTEXT_AWARE_BUILTIN(electron_browser_web_view_manager, Initialize)
