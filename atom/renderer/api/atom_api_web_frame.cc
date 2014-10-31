// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/renderer/api/atom_api_web_frame.h"

#include "atom/common/native_mate_converters/string16_converter.h"
#include "native_mate/dictionary.h"
#include "native_mate/object_template_builder.h"
#include "third_party/WebKit/public/web/WebDocument.h"
#include "third_party/WebKit/public/web/WebLocalFrame.h"
#include "third_party/WebKit/public/web/WebView.h"

#include "atom/common/node_includes.h"

namespace atom {

namespace api {

WebFrame::WebFrame()
    : web_frame_(blink::WebLocalFrame::frameForCurrentContext()) {
}

WebFrame::~WebFrame() {
}

void WebFrame::SetName(const std::string& name) {
  web_frame_->setName(blink::WebString::fromUTF8(name));
}

double WebFrame::SetZoomLevel(double level) {
  return web_frame_->view()->setZoomLevel(level);
}

double WebFrame::GetZoomLevel() const {
  return web_frame_->view()->zoomLevel();
}

double WebFrame::SetZoomFactor(double factor) {
  return blink::WebView::zoomLevelToZoomFactor(SetZoomLevel(
      blink::WebView::zoomFactorToZoomLevel(factor)));
}

double WebFrame::GetZoomFactor() const {
  return blink::WebView::zoomLevelToZoomFactor(GetZoomLevel());
}

v8::Handle<v8::Value> WebFrame::RegisterEmbedderCustomElement(
    const base::string16& name, v8::Handle<v8::Object> options) {
  blink::WebExceptionCode c = 0;
  return web_frame_->document().registerEmbedderCustomElement(name, options, c);
}

mate::ObjectTemplateBuilder WebFrame::GetObjectTemplateBuilder(
    v8::Isolate* isolate) {
  return mate::ObjectTemplateBuilder(isolate)
      .SetMethod("setName", &WebFrame::SetName)
      .SetMethod("setZoomLevel", &WebFrame::SetZoomLevel)
      .SetMethod("getZoomLevel", &WebFrame::GetZoomLevel)
      .SetMethod("setZoomFactor", &WebFrame::SetZoomFactor)
      .SetMethod("getZoomFactor", &WebFrame::GetZoomFactor)
      .SetMethod("registerEmbedderCustomElement",
                 &WebFrame::RegisterEmbedderCustomElement);
}

// static
mate::Handle<WebFrame> WebFrame::Create(v8::Isolate* isolate) {
  return CreateHandle(isolate, new WebFrame);
}

}  // namespace api

}  // namespace atom

namespace {

void Initialize(v8::Handle<v8::Object> exports, v8::Handle<v8::Value> unused,
                v8::Handle<v8::Context> context, void* priv) {
  v8::Isolate* isolate = context->GetIsolate();
  mate::Dictionary dict(isolate, exports);
  dict.Set("webFrame", atom::api::WebFrame::Create(isolate));
}

}  // namespace

NODE_MODULE_CONTEXT_AWARE_BUILTIN(atom_renderer_web_frame, Initialize)
