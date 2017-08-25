// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/renderer/api/atom_api_web_frame.h"

#include "atom/common/api/api_messages.h"
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
#include "third_party/WebKit/public/platform/WebCache.h"
#include "third_party/WebKit/public/web/WebDocument.h"
#include "third_party/WebKit/public/web/WebFrameWidget.h"
#include "third_party/WebKit/public/web/WebInputMethodController.h"
#include "third_party/WebKit/public/web/WebLocalFrame.h"
#include "third_party/WebKit/public/web/WebScriptExecutionCallback.h"
#include "third_party/WebKit/public/web/WebScriptSource.h"
#include "third_party/WebKit/public/web/WebView.h"
#include "third_party/WebKit/Source/platform/weborigin/SchemeRegistry.h"

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

  void Completed(
      const blink::WebVector<v8::Local<v8::Value>>& result) override {
    if (!callback_.is_null() && !result.IsEmpty() && !result[0].IsEmpty())
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
    : web_frame_(blink::WebLocalFrame::FrameForCurrentContext()) {
  Init(isolate);
}

WebFrame::~WebFrame() {
}

void WebFrame::SetName(const std::string& name) {
  web_frame_->SetName(blink::WebString::FromUTF8(name));
}

double WebFrame::SetZoomLevel(double level) {
  double result = 0.0;
  content::RenderView* render_view =
      content::RenderView::FromWebView(web_frame_->View());
  render_view->Send(new AtomViewHostMsg_SetTemporaryZoomLevel(
      render_view->GetRoutingID(), level, &result));
  return result;
}

double WebFrame::GetZoomLevel() const {
  double result = 0.0;
  content::RenderView* render_view =
      content::RenderView::FromWebView(web_frame_->View());
  render_view->Send(
      new AtomViewHostMsg_GetZoomLevel(render_view->GetRoutingID(), &result));
  return result;
}

double WebFrame::SetZoomFactor(double factor) {
  return blink::WebView::ZoomLevelToZoomFactor(SetZoomLevel(
      blink::WebView::ZoomFactorToZoomLevel(factor)));
}

double WebFrame::GetZoomFactor() const {
  return blink::WebView::ZoomLevelToZoomFactor(GetZoomLevel());
}

void WebFrame::SetVisualZoomLevelLimits(double min_level, double max_level) {
  web_frame_->View()->SetDefaultPageScaleLimits(min_level, max_level);
}

void WebFrame::SetLayoutZoomLevelLimits(double min_level, double max_level) {
  web_frame_->View()->ZoomLimitsChanged(min_level, max_level);
}

v8::Local<v8::Value> WebFrame::RegisterEmbedderCustomElement(
    const base::string16& name, v8::Local<v8::Object> options) {
  blink::WebExceptionCode c = 0;
  return web_frame_->GetDocument().RegisterEmbedderCustomElement(
      blink::WebString::FromUTF16(name), options, c);
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
  web_frame_->SetSpellCheckPanelHostClient(spell_check_client_.get());
  web_frame_->SetTextCheckClient(spell_check_client_.get());
}

void WebFrame::RegisterURLSchemeAsSecure(const std::string& scheme) {
  // TODO(pfrazee): Remove 2.0
  blink::SchemeRegistry::RegisterURLSchemeAsSecure(
      WTF::String::FromUTF8(scheme.data(), scheme.length()));
}

void WebFrame::RegisterURLSchemeAsBypassingCSP(const std::string& scheme) {
  // Register scheme to bypass pages's Content Security Policy.
  blink::SchemeRegistry::RegisterURLSchemeAsBypassingContentSecurityPolicy(
      WTF::String::FromUTF8(scheme.data(), scheme.length()));
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
  WTF::String privileged_scheme(
      WTF::String::FromUTF8(scheme.data(), scheme.length()));
  if (secure) {
    // TODO(pfrazee): Remove 2.0
    blink::SchemeRegistry::RegisterURLSchemeAsSecure(privileged_scheme);
  }
  if (bypassCSP) {
    blink::SchemeRegistry::RegisterURLSchemeAsBypassingContentSecurityPolicy(
        privileged_scheme);
  }
  if (allowServiceWorkers) {
    blink::SchemeRegistry::RegisterURLSchemeAsAllowingServiceWorkers(
        privileged_scheme);
  }
  if (supportFetchAPI) {
    blink::SchemeRegistry::RegisterURLSchemeAsSupportingFetchAPI(
        privileged_scheme);
  }
  if (corsEnabled) {
    blink::SchemeRegistry::RegisterURLSchemeAsCORSEnabled(privileged_scheme);
  }
}

void WebFrame::InsertText(const std::string& text) {
  web_frame_->FrameWidget()
            ->GetActiveWebInputMethodController()
            ->CommitText(blink::WebString::FromUTF8(text),
                         blink::WebVector<blink::WebCompositionUnderline>(),
                         blink::WebRange(),
                         0);
}

void WebFrame::InsertCSS(const std::string& css) {
  web_frame_->GetDocument().InsertStyleSheet(blink::WebString::FromUTF8(css));
}

void WebFrame::ExecuteJavaScript(const base::string16& code,
                                 mate::Arguments* args) {
  bool has_user_gesture = false;
  args->GetNext(&has_user_gesture);
  ScriptExecutionCallback::CompletionCallback completion_callback;
  args->GetNext(&completion_callback);
  std::unique_ptr<blink::WebScriptExecutionCallback> callback(
      new ScriptExecutionCallback(completion_callback));
  web_frame_->RequestExecuteScriptAndReturnValue(
      blink::WebScriptSource(blink::WebString::FromUTF16(code)),
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
  blink::WebCache::GetResourceTypeStats(&stats);
  return stats;
}

void WebFrame::ClearCache(v8::Isolate* isolate) {
  isolate->IdleNotificationDeadline(0.5);
  blink::WebCache::Clear();
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
      .SetMethod("insertCSS", &WebFrame::InsertCSS)
      .SetMethod("executeJavaScript", &WebFrame::ExecuteJavaScript)
      .SetMethod("getResourceUsage", &WebFrame::GetResourceUsage)
      .SetMethod("clearCache", &WebFrame::ClearCache)
      // TODO(kevinsawicki): Remove in 2.0, deprecate before then with warnings
      .SetMethod("setZoomLevelLimits", &WebFrame::SetVisualZoomLevelLimits);
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
