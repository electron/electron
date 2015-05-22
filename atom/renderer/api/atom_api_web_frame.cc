// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/renderer/api/atom_api_web_frame.h"

// This defines are required by SchemeRegistry.h.
#define ALWAYS_INLINE inline
#define OS(WTF_FEATURE) (defined WTF_OS_##WTF_FEATURE  && WTF_OS_##WTF_FEATURE)  // NOLINT
#define USE(WTF_FEATURE) (defined WTF_USE_##WTF_FEATURE  && WTF_USE_##WTF_FEATURE)  // NOLINT
#define ENABLE(WTF_FEATURE) (defined ENABLE_##WTF_FEATURE  && ENABLE_##WTF_FEATURE)  // NOLINT

#include "atom/common/native_mate_converters/string16_converter.h"
#include "atom/renderer/api/atom_api_spell_check_client.h"
#include "content/public/renderer/render_frame.h"
#include "native_mate/dictionary.h"
#include "native_mate/object_template_builder.h"
#include "third_party/WebKit/public/web/WebDocument.h"
#include "third_party/WebKit/public/web/WebLocalFrame.h"
#include "third_party/WebKit/public/web/WebView.h"
#include "third_party/WebKit/Source/platform/weborigin/SchemeRegistry.h"

#include "atom/common/node_includes.h"

namespace mate {

template<>
struct Converter<WTF::String> {
  static bool FromV8(v8::Isolate* isolate,
                     v8::Local<v8::Value> val,
                     WTF::String* out) {
    if (!val->IsString())
      return false;

    v8::String::Value s(val);
    *out = WTF::String(reinterpret_cast<const base::char16*>(*s), s.length());
    return true;
  }
};

}  // namespace mate

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

v8::Local<v8::Value> WebFrame::RegisterEmbedderCustomElement(
    const base::string16& name, v8::Local<v8::Object> options) {
  blink::WebExceptionCode c = 0;
  return web_frame_->document().registerEmbedderCustomElement(name, options, c);
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

mate::ObjectTemplateBuilder WebFrame::GetObjectTemplateBuilder(
    v8::Isolate* isolate) {
  return mate::ObjectTemplateBuilder(isolate)
      .SetMethod("setName", &WebFrame::SetName)
      .SetMethod("setZoomLevel", &WebFrame::SetZoomLevel)
      .SetMethod("getZoomLevel", &WebFrame::GetZoomLevel)
      .SetMethod("setZoomFactor", &WebFrame::SetZoomFactor)
      .SetMethod("getZoomFactor", &WebFrame::GetZoomFactor)
      .SetMethod("registerEmbedderCustomElement",
                 &WebFrame::RegisterEmbedderCustomElement)
      .SetMethod("attachGuest", &WebFrame::AttachGuest)
      .SetMethod("setSpellCheckProvider", &WebFrame::SetSpellCheckProvider)
      .SetMethod("registerUrlSchemeAsSecure",
                 &blink::SchemeRegistry::registerURLSchemeAsSecure);
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
