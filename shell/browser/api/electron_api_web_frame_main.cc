// Copyright (c) 2020 Samuel Maddock <sam@samuelmaddock.com>.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/api/electron_api_web_frame_main.h"

#include <string>
#include <utility>
#include <vector>

#include "gin/object_template_builder.h"
#include "shell/browser/browser.h"
#include "shell/common/gin_converters/frame_converter.h"
#include "shell/common/gin_converters/gurl_converter.h"
#include "shell/common/gin_helper/dictionary.h"
#include "shell/common/gin_helper/object_template_builder.h"
#include "shell/common/gin_helper/promise.h"
#include "shell/common/node_includes.h"

namespace electron {

namespace api {

gin::WrapperInfo WebFrame::kWrapperInfo = {gin::kEmbedderNativeGin};

WebFrame::WebFrame(content::RenderFrameHost* render_frame)
    : render_frame_(render_frame) {
  // TODO
}

WebFrame::~WebFrame() {
  // TODO
}

bool WebFrame::Reload() {
  return render_frame_->Reload();
}

int WebFrame::GetFrameTreeNodeID() const {
  return render_frame_->GetFrameTreeNodeId();
}

int WebFrame::GetRoutingID() const {
  return render_frame_->GetRoutingID();
}

GURL WebFrame::GetURL() const {
  return render_frame_->GetLastCommittedURL();
}

content::RenderFrameHost* WebFrame::GetParent() {
  return render_frame_->GetParent();
}

// static
gin::Handle<WebFrame> WebFrame::FromID(v8::Isolate* isolate,
                                       int render_process_id,
                                       int render_frame_id) {
  content::RenderFrameHost* render_frame =
      content::RenderFrameHost::FromID(render_process_id, render_frame_id);

  auto handle = gin::CreateHandle(isolate, new WebFrame(render_frame));

  return handle;
}

// static
gin::Handle<WebFrame> WebFrame::From(
    v8::Isolate* isolate,
    content::RenderFrameHost* render_frame_host) {
  auto handle = gin::CreateHandle(isolate, new WebFrame(render_frame_host));

  return handle;
}

gin::ObjectTemplateBuilder WebFrame::GetObjectTemplateBuilder(
    v8::Isolate* isolate) {
  return gin::Wrappable<WebFrame>::GetObjectTemplateBuilder(isolate)
      .SetMethod("reload", &WebFrame::Reload)
      .SetProperty("frameTreeNodeId", &WebFrame::GetFrameTreeNodeID)
      .SetProperty("routingId", &WebFrame::GetRoutingID)
      .SetProperty("url", &WebFrame::GetURL)
      .SetProperty("parent", &WebFrame::GetParent);
}

const char* WebFrame::GetTypeName() {
  return "WebFrame";
}

}  // namespace api

}  // namespace electron

namespace {

using electron::api::WebFrame;

v8::Local<v8::Value> FromID(gin_helper::ErrorThrower thrower,
                            int render_process_id,
                            int render_frame_id) {
  if (!electron::Browser::Get()->is_ready()) {
    thrower.ThrowError("WebFrame can only be received when app is ready");
    return v8::Null(thrower.isolate());
  }

  return WebFrame::FromID(thrower.isolate(), render_process_id, render_frame_id)
      .ToV8();
}

void Initialize(v8::Local<v8::Object> exports,
                v8::Local<v8::Value> unused,
                v8::Local<v8::Context> context,
                void* priv) {
  v8::Isolate* isolate = context->GetIsolate();
  gin_helper::Dictionary dict(isolate, exports);
  dict.SetMethod("fromId", &FromID);
}

}  // namespace

NODE_LINKED_MODULE_CONTEXT_AWARE(electron_browser_web_frame_main, Initialize)
