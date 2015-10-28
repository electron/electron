// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/renderer/api/atom_api_web_frame.h"

#include "atom/common/api/api_messages.h"
#include "atom/common/native_mate_converters/callback.h"
#include "atom/common/native_mate_converters/gfx_converter.h"
#include "atom/common/native_mate_converters/string16_converter.h"
#include "atom/renderer/api/atom_api_spell_check_client.h"
#include "content/public/renderer/render_frame.h"
#include "content/public/renderer/render_view.h"
#include "native_mate/dictionary.h"
#include "native_mate/object_template_builder.h"
#include "third_party/WebKit/public/web/WebDocument.h"
#include "third_party/WebKit/public/web/WebLocalFrame.h"
#include "third_party/WebKit/public/web/WebSecurityPolicy.h"
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
  auto render_view = content::RenderView::FromWebView(web_frame_->view());
  // Notify guests if any for zoom level change.
  render_view->Send(
      new AtomViewHostMsg_ZoomLevelChanged(MSG_ROUTING_NONE, level));
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

void WebFrame::SetZoomLevelLimits(double min_level, double max_level) {
  web_frame_->view()->setDefaultPageScaleLimits(min_level, max_level);
}

v8::Local<v8::Value> WebFrame::RegisterEmbedderCustomElement(
    const base::string16& name, v8::Local<v8::Object> options) {
  blink::WebExceptionCode c = 0;
  return web_frame_->document().registerEmbedderCustomElement(name, options, c);
}

void WebFrame::RegisterElementResizeCallback(
    int element_instance_id,
    const GuestViewContainer::ResizeCallback& callback) {
  auto guest_view_container = GuestViewContainer::FromID(element_instance_id);
  if (guest_view_container)
    guest_view_container->RegisterElementResizeCallback(callback);
}

void WebFrame::AttachGuest(int id) {
  content::RenderFrame::FromWebFrame(web_frame_)->AttachGuest(id);
}

void WebFrame::SetSpellCheckProvider(mate::Arguments* args,
                                     const std::string& language,
                                     bool auto_spell_correct_turned_on,
                                     v8::Local<v8::Object> provider) {
  if (!provider->Has(mate::StringToV8(args->isolate(), "spellCheck"))) {
    args->ThrowError("\"spellCheck\" has to be defined");
    return;
  }

  spell_check_client_.reset(new SpellCheckClient(
      language, auto_spell_correct_turned_on, args->isolate(), provider));
  web_frame_->view()->setSpellCheckClient(spell_check_client_.get());
}

void WebFrame::RegisterURLSchemeAsSecure(const std::string& scheme) {
  // Register scheme to secure list (https, wss, data).
  blink::WebSecurityPolicy::registerURLSchemeAsSecure(
      blink::WebString::fromUTF8(scheme));
}

void WebFrame::RegisterURLSchemeAsBypassingCsp(const std::string& scheme) {
  // Register scheme to bypass pages's Content Security Policy.
  blink::WebSecurityPolicy::registerURLSchemeAsBypassingContentSecurityPolicy(
      blink::WebString::fromUTF8(scheme));
}

void WebFrame::RegisterURLSchemeAsPrivileged(const std::string& scheme) {
  // Register scheme to privileged list (https, wss, data, chrome-extension)
  blink::WebString privileged_scheme(blink::WebString::fromUTF8(scheme));
  blink::WebSecurityPolicy::registerURLSchemeAsSecure(privileged_scheme);
  blink::WebSecurityPolicy::registerURLSchemeAsBypassingContentSecurityPolicy(
      privileged_scheme);
  blink::WebSecurityPolicy::registerURLSchemeAsAllowingServiceWorkers(
      privileged_scheme);
}

mate::ObjectTemplateBuilder WebFrame::GetObjectTemplateBuilder(
    v8::Isolate* isolate) {
  return mate::ObjectTemplateBuilder(isolate)
      .SetMethod("setName", &WebFrame::SetName)
      .SetMethod("setZoomLevel", &WebFrame::SetZoomLevel)
      .SetMethod("getZoomLevel", &WebFrame::GetZoomLevel)
      .SetMethod("setZoomFactor", &WebFrame::SetZoomFactor)
      .SetMethod("getZoomFactor", &WebFrame::GetZoomFactor)
      .SetMethod("setZoomLevelLimits", &WebFrame::SetZoomLevelLimits)
      .SetMethod("registerEmbedderCustomElement",
                 &WebFrame::RegisterEmbedderCustomElement)
      .SetMethod("registerElementResizeCallback",
                 &WebFrame::RegisterElementResizeCallback)
      .SetMethod("attachGuest", &WebFrame::AttachGuest)
      .SetMethod("setSpellCheckProvider", &WebFrame::SetSpellCheckProvider)
      .SetMethod("registerUrlSchemeAsSecure",
                 &WebFrame::RegisterURLSchemeAsSecure)
      .SetMethod("registerUrlSchemeAsBypassingCsp",
                 &WebFrame::RegisterURLSchemeAsBypassingCsp)
      .SetMethod("registerUrlSchemeAsPrivileged",
                 &WebFrame::RegisterURLSchemeAsPrivileged);
}

// static
mate::Handle<WebFrame> WebFrame::Create(v8::Isolate* isolate) {
  return CreateHandle(isolate, new WebFrame);
}

}  // namespace api

}  // namespace atom

namespace {

void Initialize(v8::Local<v8::Object> exports, v8::Local<v8::Value> unused,
                v8::Local<v8::Context> context, void* priv) {
  v8::Isolate* isolate = context->GetIsolate();
  mate::Dictionary dict(isolate, exports);
  dict.Set("webFrame", atom::api::WebFrame::Create(isolate));
}

}  // namespace

NODE_MODULE_CONTEXT_AWARE_BUILTIN(atom_renderer_web_frame, Initialize)
