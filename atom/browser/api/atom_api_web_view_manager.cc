// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/api/atom_api_web_contents.h"
#include "atom/browser/atom_browser_context.h"
#include "atom/browser/web_view_manager.h"
#include "atom/common/native_mate_converters/gurl_converter.h"
#include "native_mate/dictionary.h"
#include "net/base/filename_util.h"

#include "atom/common/node_includes.h"

namespace mate {

template<>
struct Converter<content::WebContents*> {
  static bool FromV8(v8::Isolate* isolate, v8::Handle<v8::Value> val,
                     content::WebContents** out) {
    atom::api::WebContents* contents;
    if (!Converter<atom::api::WebContents*>::FromV8(isolate, val, &contents))
      return false;
    *out = contents->web_contents();
    return true;
  }
};

template<>
struct Converter<atom::WebViewManager::WebViewInfo> {
  static bool FromV8(v8::Isolate* isolate, v8::Handle<v8::Value> val,
                     atom::WebViewManager::WebViewInfo* out) {
    Dictionary options;
    if (!ConvertFromV8(isolate, val, &options))
      return false;

    GURL preload_url;
    if (!options.Get("preloadUrl", &preload_url))
      return false;

    if (!preload_url.is_empty() &&
        !net::FileURLToFilePath(preload_url, &(out->preload_script)))
      return false;

    return options.Get("nodeIntegration", &(out->node_integration)) &&
           options.Get("plugins", &(out->plugins)) &&
           options.Get("disableWebSecurity", &(out->disable_web_security));
  }
};

}  // namespace mate

namespace {

void Initialize(v8::Handle<v8::Object> exports, v8::Handle<v8::Value> unused,
                v8::Handle<v8::Context> context, void* priv) {
  using atom::WebViewManager;
  auto manager = static_cast<WebViewManager*>(
      atom::AtomBrowserContext::Get()->GetGuestManager());
  mate::Dictionary dict(context->GetIsolate(), exports);
  dict.SetMethod("addGuest",
                 base::Bind(&WebViewManager::AddGuest,
                            base::Unretained(manager)));
  dict.SetMethod("removeGuest",
                 base::Bind(&WebViewManager::RemoveGuest,
                            base::Unretained(manager)));
}

}  // namespace

NODE_MODULE_CONTEXT_AWARE_BUILTIN(atom_browser_web_view_manager, Initialize)
