// Copyright (c) 2014 GitHub, Inc. All rights reserved.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/web_view/web_view_manager.h"

#include "atom/browser/api/atom_api_web_contents.h"
#include "atom/browser/atom_browser_context.h"
#include "atom/browser/web_view/web_view_renderer_state.h"
#include "base/bind.h"
#include "base/stl_util.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/render_process_host.h"
#include "native_mate/dictionary.h"
#include "native_mate/object_template_builder.h"

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

}  // namespace mate

namespace atom {

WebViewManager::WebViewManager(content::BrowserContext* context) {
}

WebViewManager::~WebViewManager() {
}

void WebViewManager::AddGuest(int guest_instance_id,
                              content::WebContents* embedder,
                              content::WebContents* web_contents,
                              bool node_integration) {
  web_contents_map_[guest_instance_id] = { web_contents, embedder };

  WebViewRendererState::WebViewInfo web_view_info = { node_integration };
  content::BrowserThread::PostTask(
      content::BrowserThread::IO,
      FROM_HERE,
      base::Bind(&WebViewRendererState::AddGuest,
                 base::Unretained(WebViewRendererState::GetInstance()),
                 web_contents->GetRenderProcessHost()->GetID(),
                 web_view_info));
}

void WebViewManager::RemoveGuest(int guest_instance_id) {
  auto web_contents = web_contents_map_[guest_instance_id].web_contents;
  content::BrowserThread::PostTask(
      content::BrowserThread::IO, FROM_HERE,
      base::Bind(
          &WebViewRendererState::RemoveGuest,
          base::Unretained(WebViewRendererState::GetInstance()),
          web_contents->GetRenderProcessHost()->GetID()));

  web_contents_map_.erase(guest_instance_id);
}

void WebViewManager::MaybeGetGuestByInstanceIDOrKill(
    int guest_instance_id,
    int embedder_render_process_id,
    const GuestByInstanceIDCallback& callback) {
  if (ContainsKey(web_contents_map_, guest_instance_id))
    callback.Run(web_contents_map_[guest_instance_id].web_contents);
  else
    callback.Run(nullptr);
}

bool WebViewManager::ForEachGuest(content::WebContents* embedder_web_contents,
                                  const GuestCallback& callback) {
  for (auto& item : web_contents_map_)
    if (item.second.embedder == embedder_web_contents &&
        callback.Run(item.second.web_contents))
      return true;
  return false;
}

}  // namespace atom

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
