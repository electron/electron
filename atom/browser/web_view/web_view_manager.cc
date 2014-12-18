// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/web_view/web_view_manager.h"

#include "atom/browser/api/atom_api_web_contents.h"
#include "atom/browser/atom_browser_context.h"
#include "atom/browser/web_view/web_view_renderer_state.h"
#include "atom/common/native_mate_converters/gurl_converter.h"
#include "base/bind.h"
#include "base/stl_util.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/render_process_host.h"
#include "native_mate/dictionary.h"
#include "native_mate/object_template_builder.h"
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
struct Converter<atom::WebViewManager::WebViewOptions> {
  static bool FromV8(v8::Isolate* isolate, v8::Handle<v8::Value> val,
                     atom::WebViewManager::WebViewOptions* out) {
    Dictionary options;
    if (!ConvertFromV8(isolate, val, &options))
      return false;
    return options.Get("nodeIntegration", &(out->node_integration)) &&
           options.Get("plugins", &(out->plugins)) &&
           options.Get("preloadUrl", &(out->preload_url)) &&
           options.Get("disableWebSecurity", &(out->disable_web_security));
  }
};

}  // namespace mate

namespace atom {

WebViewManager::WebViewManager(content::BrowserContext* context) {
}

WebViewManager::~WebViewManager() {
}

void WebViewManager::AddGuest(int guest_instance_id,
                              int element_instance_id,
                              content::WebContents* embedder,
                              content::WebContents* web_contents,
                              const WebViewOptions& options) {
  web_contents_map_[guest_instance_id] = { web_contents, embedder };

  WebViewRendererState::WebViewInfo web_view_info = {
    guest_instance_id,
    embedder,
    options.node_integration,
    options.plugins,
    options.disable_web_security,
  };
  net::FileURLToFilePath(options.preload_url, &web_view_info.preload_script);
  content::BrowserThread::PostTask(
      content::BrowserThread::IO,
      FROM_HERE,
      base::Bind(&WebViewRendererState::AddGuest,
                 base::Unretained(WebViewRendererState::GetInstance()),
                 web_contents->GetRenderProcessHost()->GetID(),
                 web_view_info));

  // Map the element in embedder to guest.
  ElementInstanceKey key(embedder, element_instance_id);
  element_instance_id_to_guest_map_[key] = guest_instance_id;
}

void WebViewManager::RemoveGuest(int guest_instance_id) {
  if (!ContainsKey(web_contents_map_, guest_instance_id)) {
    return;
  }

  auto web_contents = web_contents_map_[guest_instance_id].web_contents;

  content::BrowserThread::PostTask(
      content::BrowserThread::IO, FROM_HERE,
      base::Bind(
          &WebViewRendererState::RemoveGuest,
          base::Unretained(WebViewRendererState::GetInstance()),
          web_contents->GetRenderProcessHost()->GetID()));

  web_contents_map_.erase(guest_instance_id);

  // Remove the record of element in embedder too.
  for (const auto& element : element_instance_id_to_guest_map_)
    if (element.second == guest_instance_id) {
      element_instance_id_to_guest_map_.erase(element.first);
      break;
    }
}

content::WebContents* WebViewManager::GetGuestByInstanceID(
    content::WebContents* embedder,
    int element_instance_id) {
  ElementInstanceKey key(embedder, element_instance_id);
  if (!ContainsKey(element_instance_id_to_guest_map_, key))
    return nullptr;

  int guest_instance_id = element_instance_id_to_guest_map_[key];
  if (ContainsKey(web_contents_map_, guest_instance_id))
    return web_contents_map_[guest_instance_id].web_contents;
  else
    return nullptr;
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
