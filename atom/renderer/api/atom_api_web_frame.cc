// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/renderer/api/atom_api_web_frame.h"

#include "atom/common/api/event_emitter_caller.h"
#include "atom/common/native_mate_converters/blink_converter.h"
#include "atom/common/native_mate_converters/callback.h"
#include "atom/common/native_mate_converters/gfx_converter.h"
#include "atom/common/native_mate_converters/string16_converter.h"
#include "atom/renderer/api/atom_api_spell_check_client.h"
#include "base/memory/memory_pressure_listener.h"
#include "content/public/renderer/render_frame.h"
#include "content/public/renderer/render_view.h"
#include "native_mate/dictionary.h"
#include "native_mate/object_template_builder.h"
#include "third_party/WebKit/public/web/WebCache.h"
#include "third_party/WebKit/public/web/WebDocument.h"
#include "third_party/WebKit/public/web/WebLocalFrame.h"
#include "third_party/WebKit/public/web/WebScriptExecutionCallback.h"
#include "third_party/WebKit/public/web/WebScriptSource.h"
#include "third_party/WebKit/public/web/WebSecurityPolicy.h"
#include "third_party/WebKit/public/web/WebView.h"

#include "atom/common/node_includes.h"

namespace atom {

namespace api {

namespace {

class ScriptExecutionCallback : public blink::WebScriptExecutionCallback {
 public:
  using CompletionCallback =
      base::Callback<void(
          const v8::Local<v8::Value>& result)>;

  explicit ScriptExecutionCallback(const CompletionCallback& callback)
      : callback_(callback) {}
  ~ScriptExecutionCallback() override {}

  void completed(
      const blink::WebVector<v8::Local<v8::Value>>& result) override {
    if (!callback_.is_null() && !result.isEmpty() && !result[0].IsEmpty())
      // Right now only single results per frame is supported.
      callback_.Run(result[0]);
    delete this;
  }

 private:
  CompletionCallback callback_;

  DISALLOW_COPY_AND_ASSIGN(ScriptExecutionCallback);
};

}  // namespace

WebFrame::WebFrame(v8::Isolate* isolate)
    : web_frame_(blink::WebLocalFrame::frameForCurrentContext()) {
  Init(isolate);
}

WebFrame::~WebFrame() {
}

void WebFrame::SetName(const std::string& name) {
  web_frame_->setName(blink::WebString::fromUTF8(name));
}

double WebFrame::SetZoomLevel(double level) {
  double ret = web_frame_->view()->setZoomLevel(level);
  mate::EmitEvent(isolate(), GetWrapper(), "zoom-level-changed", ret);
  return ret;
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

void WebFrame::SetVisualZoomLevelLimits(double min_level, double max_level) {
  web_frame_->view()->setDefaultPageScaleLimits(min_level, max_level);
}

void WebFrame::SetLayoutZoomLevelLimits(double min_level, double max_level) {
  web_frame_->view()->zoomLimitsChanged(min_level, max_level);
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

void WebFrame::DetachGuest(int id) {
  content::RenderFrame::FromWebFrame(web_frame_)->DetachGuest(id);
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

void WebFrame::RegisterURLSchemeAsBypassingCSP(const std::string& scheme) {
  // Register scheme to bypass pages's Content Security Policy.
  blink::WebSecurityPolicy::registerURLSchemeAsBypassingContentSecurityPolicy(
      blink::WebString::fromUTF8(scheme));
}

void WebFrame::RegisterURLSchemeAsPrivileged(const std::string& scheme,
                                             mate::Arguments* args) {
  // Read optional flags
  bool secure = true;
  bool bypassCSP = true;
  bool allowServiceWorkers = true;
  bool supportFetchAPI = true;
  bool corsEnabled = true;
  if (args->Length() == 2) {
    mate::Dictionary options;
    if (args->GetNext(&options)) {
      options.Get("secure", &secure);
      options.Get("bypassCSP", &bypassCSP);
      options.Get("allowServiceWorkers", &allowServiceWorkers);
      options.Get("supportFetchAPI", &supportFetchAPI);
      options.Get("corsEnabled", &corsEnabled);
    }
  }
  // Register scheme to privileged list (https, wss, data, chrome-extension)
  blink::WebString privileged_scheme(blink::WebString::fromUTF8(scheme));
  if (secure) {
    blink::WebSecurityPolicy::registerURLSchemeAsSecure(privileged_scheme);
  }
  if (bypassCSP) {
    blink::WebSecurityPolicy::registerURLSchemeAsBypassingContentSecurityPolicy(
        privileged_scheme);
  }
  if (allowServiceWorkers) {
    blink::WebSecurityPolicy::registerURLSchemeAsAllowingServiceWorkers(
        privileged_scheme);
  }
  if (supportFetchAPI) {
    blink::WebSecurityPolicy::registerURLSchemeAsSupportingFetchAPI(
        privileged_scheme);
  }
  if (corsEnabled) {
    blink::WebSecurityPolicy::registerURLSchemeAsCORSEnabled(privileged_scheme);
  }
}

void WebFrame::InsertText(const std::string& text) {
  web_frame_->insertText(blink::WebString::fromUTF8(text));
}

void WebFrame::ExecuteJavaScript(const base::string16& code,
                                 mate::Arguments* args) {
  bool has_user_gesture = false;
  args->GetNext(&has_user_gesture);
  ScriptExecutionCallback::CompletionCallback completion_callback;
  args->GetNext(&completion_callback);
  std::unique_ptr<blink::WebScriptExecutionCallback> callback(
      new ScriptExecutionCallback(completion_callback));
  web_frame_->requestExecuteScriptAndReturnValue(
      blink::WebScriptSource(code),
      has_user_gesture,
      callback.release());
}

// static
mate::Handle<WebFrame> WebFrame::Create(v8::Isolate* isolate) {
  return mate::CreateHandle(isolate, new WebFrame(isolate));
}

blink::WebCache::ResourceTypeStats WebFrame::GetResourceUsage(
    v8::Isolate* isolate) {
  blink::WebCache::ResourceTypeStats stats;
  blink::WebCache::getResourceTypeStats(&stats);
  return stats;
}

void WebFrame::ClearCache(v8::Isolate* isolate) {
  isolate->IdleNotificationDeadline(0.5);
  blink::WebCache::clear();
  base::MemoryPressureListener::NotifyMemoryPressure(
    base::MemoryPressureListener::MEMORY_PRESSURE_LEVEL_CRITICAL);
}

// static
void WebFrame::BuildPrototype(
    v8::Isolate* isolate, v8::Local<v8::FunctionTemplate> prototype) {
  prototype->SetClassName(mate::StringToV8(isolate, "WebFrame"));
  mate::ObjectTemplateBuilder(isolate, prototype->PrototypeTemplate())
      .SetMethod("setName", &WebFrame::SetName)
      .SetMethod("setZoomLevel", &WebFrame::SetZoomLevel)
      .SetMethod("getZoomLevel", &WebFrame::GetZoomLevel)
      .SetMethod("setZoomFactor", &WebFrame::SetZoomFactor)
      .SetMethod("getZoomFactor", &WebFrame::GetZoomFactor)
      .SetMethod("setZoomLevelLimits", &WebFrame::SetVisualZoomLevelLimits)
      .SetMethod("setVisualZoomLevelLimits",
                 &WebFrame::SetVisualZoomLevelLimits)
      .SetMethod("setLayoutZoomLevelLimits",
                 &WebFrame::SetLayoutZoomLevelLimits)
      .SetMethod("registerEmbedderCustomElement",
                 &WebFrame::RegisterEmbedderCustomElement)
      .SetMethod("registerElementResizeCallback",
                 &WebFrame::RegisterElementResizeCallback)
      .SetMethod("attachGuest", &WebFrame::AttachGuest)
      .SetMethod("detachGuest", &WebFrame::DetachGuest)
      .SetMethod("setSpellCheckProvider", &WebFrame::SetSpellCheckProvider)
      .SetMethod("registerURLSchemeAsSecure",
                 &WebFrame::RegisterURLSchemeAsSecure)
      .SetMethod("registerURLSchemeAsBypassingCSP",
                 &WebFrame::RegisterURLSchemeAsBypassingCSP)
      .SetMethod("registerURLSchemeAsPrivileged",
                 &WebFrame::RegisterURLSchemeAsPrivileged)
      .SetMethod("insertText", &WebFrame::InsertText)
      .SetMethod("executeJavaScript", &WebFrame::ExecuteJavaScript)
      .SetMethod("getResourceUsage", &WebFrame::GetResourceUsage)
      .SetMethod("clearCache", &WebFrame::ClearCache);
}

}  // namespace api

}  // namespace atom

namespace {

using atom::api::WebFrame;

void Initialize(v8::Local<v8::Object> exports, v8::Local<v8::Value> unused,
                v8::Local<v8::Context> context, void* priv) {
  v8::Isolate* isolate = context->GetIsolate();
  mate::Dictionary dict(isolate, exports);
  dict.Set("webFrame", WebFrame::Create(isolate));
  dict.Set("WebFrame", WebFrame::GetConstructor(isolate)->GetFunction());
}

}  // namespace

NODE_MODULE_CONTEXT_AWARE_BUILTIN(atom_renderer_web_frame, Initialize)
