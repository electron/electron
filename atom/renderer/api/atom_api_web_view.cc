// Copyright (c) 2014 GitHub, Inc. All rights reserved.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/renderer/api/atom_api_web_view.h"

#include "native_mate/dictionary.h"
#include "native_mate/object_template_builder.h"
#include "third_party/WebKit/public/web/WebFrame.h"
#include "third_party/WebKit/public/web/WebView.h"

#include "atom/common/node_includes.h"

namespace atom {

namespace api {

namespace {

WebKit::WebView* GetCurrentWebView() {
  WebKit::WebFrame* frame = WebKit::WebFrame::frameForCurrentContext();
  if (!frame)
    return NULL;
  return frame->view();
}

}  // namespace

WebView::WebView() : web_view_(GetCurrentWebView()) {
}

WebView::~WebView() {
}

double WebView::SetZoomLevel(double level) {
  return web_view_->setZoomLevel(level);
}

double WebView::GetZoomLevel() const {
  return web_view_->zoomLevel();
}

double WebView::SetZoomFactor(double factor) {
  return WebKit::WebView::zoomLevelToZoomFactor(SetZoomLevel(
      WebKit::WebView::zoomFactorToZoomLevel(factor)));
}

double WebView::GetZoomFactor() const {
  return WebKit::WebView::zoomLevelToZoomFactor(GetZoomLevel());
}

mate::ObjectTemplateBuilder WebView::GetObjectTemplateBuilder(
    v8::Isolate* isolate) {
  return mate::ObjectTemplateBuilder(isolate)
      .SetMethod("setZoomLevel", &WebView::SetZoomLevel)
      .SetMethod("getZoomLevel", &WebView::GetZoomLevel)
      .SetMethod("setZoomFactor", &WebView::SetZoomFactor)
      .SetMethod("getZoomFactor", &WebView::GetZoomFactor);
}

// static
mate::Handle<WebView> WebView::Create(v8::Isolate* isolate) {
  return CreateHandle(isolate, new WebView);
}

}  // namespace api

}  // namespace atom

namespace {

void Initialize(v8::Handle<v8::Object> exports) {
  v8::Isolate* isolate = v8::Isolate::GetCurrent();
  mate::Dictionary dict(isolate, exports);
  dict.Set("webView", atom::api::WebView::Create(isolate));
}

}  // namespace

NODE_MODULE(atom_renderer_web_view, Initialize)
