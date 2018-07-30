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
#include "atom/renderer/guest_view_container.h"
#include "base/memory/memory_pressure_listener.h"
#include "content/public/renderer/render_frame.h"
#include "content/public/renderer/render_frame_observer.h"
#include "content/public/renderer/render_frame_visitor.h"
#include "content/public/renderer/render_view.h"
#include "native_mate/dictionary.h"
#include "native_mate/object_template_builder.h"
#include "third_party/WebKit/Source/platform/weborigin/SchemeRegistry.h"
#include "third_party/WebKit/public/platform/WebCache.h"
#include "third_party/WebKit/public/web/WebDocument.h"
#include "third_party/WebKit/public/web/WebElement.h"
#include "third_party/WebKit/public/web/WebFrameWidget.h"
#include "third_party/WebKit/public/web/WebImeTextSpan.h"
#include "third_party/WebKit/public/web/WebInputMethodController.h"
#include "third_party/WebKit/public/web/WebLocalFrame.h"
#include "third_party/WebKit/public/web/WebScriptExecutionCallback.h"
#include "third_party/WebKit/public/web/WebScriptSource.h"
#include "third_party/WebKit/public/web/WebView.h"

#include "atom/common/node_includes.h"

namespace mate {

template <>
struct Converter<blink::WebLocalFrame::ScriptExecutionType> {
  static bool FromV8(v8::Isolate* isolate,
                     v8::Local<v8::Value> val,
                     blink::WebLocalFrame::ScriptExecutionType* out) {
    std::string execution_type;
    if (!ConvertFromV8(isolate, val, &execution_type))
      return false;
    if (execution_type == "asynchronous") {
      *out = blink::WebLocalFrame::kAsynchronous;
    } else if (execution_type == "asynchronousBlockingOnload") {
      *out = blink::WebLocalFrame::kAsynchronousBlockingOnload;
    } else if (execution_type == "synchronous") {
      *out = blink::WebLocalFrame::kSynchronous;
    } else {
      return false;
    }
    return true;
  }
};

}  // namespace mate

namespace atom {

namespace api {

namespace {

content::RenderFrame* GetRenderFrame(v8::Local<v8::Value> value) {
  v8::Local<v8::Context> context =
      v8::Local<v8::Object>::Cast(value)->CreationContext();
  if (context.IsEmpty())
    return nullptr;
  blink::WebLocalFrame* frame = blink::WebLocalFrame::FrameForContext(context);
  if (!frame)
    return nullptr;
  return content::RenderFrame::FromWebFrame(frame);
}

class RenderFrameStatus : public content::RenderFrameObserver {
 public:
  explicit RenderFrameStatus(content::RenderFrame* render_frame)
      : content::RenderFrameObserver(render_frame) {}
  ~RenderFrameStatus() final {}

  bool is_ok() { return render_frame() != nullptr; }

  // RenderFrameObserver implementation.
  void OnDestruct() final {}
};

class ScriptExecutionCallback : public blink::WebScriptExecutionCallback {
 public:
  using CompletionCallback =
      base::Callback<void(const v8::Local<v8::Value>& result)>;

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

class FrameSpellChecker : public content::RenderFrameVisitor {
 public:
  explicit FrameSpellChecker(SpellCheckClient* spell_check_client,
                             content::RenderFrame* main_frame)
      : spell_check_client_(spell_check_client), main_frame_(main_frame) {}
  ~FrameSpellChecker() override {
    spell_check_client_ = nullptr;
    main_frame_ = nullptr;
  }
  bool Visit(content::RenderFrame* render_frame) override {
    auto* view = render_frame->GetRenderView();
    if (view->GetMainRenderFrame() == main_frame_ ||
        (render_frame->IsMainFrame() && render_frame == main_frame_)) {
      render_frame->GetWebFrame()->SetTextCheckClient(spell_check_client_);
    }
    return true;
  }

 private:
  SpellCheckClient* spell_check_client_;
  content::RenderFrame* main_frame_;
  DISALLOW_COPY_AND_ASSIGN(FrameSpellChecker);
};

}  // namespace

WebFrame::WebFrame(v8::Isolate* isolate)
    : web_frame_(blink::WebLocalFrame::FrameForCurrentContext()) {
  Init(isolate);
}

WebFrame::WebFrame(v8::Isolate* isolate, blink::WebLocalFrame* blink_frame)
    : web_frame_(blink_frame) {
  Init(isolate);
}

WebFrame::~WebFrame() {}

void WebFrame::SetName(const std::string& name) {
  web_frame_->SetName(blink::WebString::FromUTF8(name));
}

double WebFrame::SetZoomLevel(double level) {
  double result = 0.0;
  content::RenderFrame* render_frame =
      content::RenderFrame::FromWebFrame(web_frame_);
  render_frame->Send(new AtomFrameHostMsg_SetTemporaryZoomLevel(
      render_frame->GetRoutingID(), level, &result));
  return result;
}

double WebFrame::GetZoomLevel() const {
  double result = 0.0;
  content::RenderFrame* render_frame =
      content::RenderFrame::FromWebFrame(web_frame_);
  render_frame->Send(
      new AtomFrameHostMsg_GetZoomLevel(render_frame->GetRoutingID(), &result));
  return result;
}

double WebFrame::SetZoomFactor(double factor) {
  return blink::WebView::ZoomLevelToZoomFactor(
      SetZoomLevel(blink::WebView::ZoomFactorToZoomLevel(factor)));
}

double WebFrame::GetZoomFactor() const {
  return blink::WebView::ZoomLevelToZoomFactor(GetZoomLevel());
}

void WebFrame::SetVisualZoomLevelLimits(double min_level, double max_level) {
  web_frame_->View()->SetDefaultPageScaleLimits(min_level, max_level);
  web_frame_->View()->SetIgnoreViewportTagScaleLimits(true);
}

void WebFrame::SetLayoutZoomLevelLimits(double min_level, double max_level) {
  web_frame_->View()->ZoomLimitsChanged(min_level, max_level);
}

v8::Local<v8::Value> WebFrame::RegisterEmbedderCustomElement(
    const base::string16& name,
    v8::Local<v8::Object> options) {
  return web_frame_->GetDocument().RegisterEmbedderCustomElement(
      blink::WebString::FromUTF16(name), options);
}

void WebFrame::RegisterElementResizeCallback(
    int element_instance_id,
    const GuestViewContainer::ResizeCallback& callback) {
  auto* guest_view_container = GuestViewContainer::FromID(element_instance_id);
  if (guest_view_container)
    guest_view_container->RegisterElementResizeCallback(callback);
}

int WebFrame::AttachIframeGuest(int element_instance_id,
                                v8::Local<v8::Value> content_window) {
  // Get the WebLocalFrame before (possibly) executing any user-space JS while
  // getting the |params|. We track the status of the RenderFrame via an
  // observer in case it is deleted during user code execution.
  content::RenderFrame* render_frame = GetRenderFrame(content_window);
  RenderFrameStatus render_frame_status(render_frame);

  if (!render_frame_status.is_ok())
    return -1;

  blink::WebLocalFrame* frame = render_frame->GetWebFrame();
  // Parent must exist.
  blink::WebFrame* parent_frame = frame->Parent();
  DCHECK(parent_frame);
  DCHECK(parent_frame->IsWebLocalFrame());

  content::RenderFrame* embedder_parent_frame =
      content::RenderFrame::FromWebFrame(parent_frame->ToWebLocalFrame());

  // Create a GuestViewContainer if it does not exist.
  // An element instance ID uniquely identifies an GuestViewContainer
  // within a RenderView.
  auto* guest_view_container = GuestViewContainer::FromID(element_instance_id);
  // This is the first time we hear about the |element_instance_id|.
  DCHECK(!guest_view_container);
  // The <webview> element's GC takes ownership of |guest_view_container|.
  guest_view_container = new GuestViewContainer(embedder_parent_frame);
  guest_view_container->SetElementInstanceID(element_instance_id);

  return render_frame->GetRoutingID();
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

  auto client = std::make_unique<SpellCheckClient>(
      language, auto_spell_correct_turned_on, args->isolate(), provider);
  // Set spellchecker for all live frames in the same process or
  // in the sandbox mode for all live sub frames to this WebFrame.
  FrameSpellChecker spell_checker(
      client.get(), content::RenderFrame::FromWebFrame(web_frame_));
  content::RenderFrame::ForEach(&spell_checker);
  spell_check_client_.swap(client);
  web_frame_->SetSpellCheckPanelHostClient(spell_check_client_.get());
}

void WebFrame::RegisterURLSchemeAsBypassingCSP(const std::string& scheme) {
  // Register scheme to bypass pages's Content Security Policy.
  blink::SchemeRegistry::RegisterURLSchemeAsBypassingContentSecurityPolicy(
      WTF::String::FromUTF8(scheme.data(), scheme.length()));
}

void WebFrame::RegisterURLSchemeAsPrivileged(const std::string& scheme,
                                             mate::Arguments* args) {
  // TODO(deepak1556): blink::SchemeRegistry methods should be called
  // before any renderer threads are created. Fixing this would break
  // current api. Change it with 2.0.

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
  web_frame_->FrameWidget()->GetActiveWebInputMethodController()->CommitText(
      blink::WebString::FromUTF8(text),
      blink::WebVector<blink::WebImeTextSpan>(), blink::WebRange(), 0);
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
      has_user_gesture, callback.release());
}

void WebFrame::ExecuteJavaScriptInIsolatedWorld(
    int world_id,
    const std::vector<mate::Dictionary>& scripts,
    mate::Arguments* args) {
  std::vector<blink::WebScriptSource> sources;

  for (const auto& script : scripts) {
    base::string16 code;
    base::string16 url;
    int start_line = 1;
    script.Get("url", &url);
    script.Get("startLine", &start_line);

    if (!script.Get("code", &code)) {
      args->ThrowError("Invalid 'code'");
      return;
    }

    sources.emplace_back(
        blink::WebScriptSource(blink::WebString::FromUTF16(code),
                               blink::WebURL(GURL(url)), start_line));
  }

  bool has_user_gesture = false;
  args->GetNext(&has_user_gesture);

  blink::WebLocalFrame::ScriptExecutionType scriptExecutionType =
      blink::WebLocalFrame::kSynchronous;
  args->GetNext(&scriptExecutionType);

  ScriptExecutionCallback::CompletionCallback completion_callback;
  args->GetNext(&completion_callback);
  std::unique_ptr<blink::WebScriptExecutionCallback> callback(
      new ScriptExecutionCallback(completion_callback));

  web_frame_->RequestExecuteScriptInIsolatedWorld(
      world_id, &sources.front(), sources.size(), has_user_gesture,
      scriptExecutionType, callback.release());
}

void WebFrame::SetIsolatedWorldSecurityOrigin(int world_id,
                                              const std::string& origin_url) {
  web_frame_->SetIsolatedWorldSecurityOrigin(
      world_id, blink::WebSecurityOrigin::CreateFromString(
                    blink::WebString::FromUTF8(origin_url)));
}

void WebFrame::SetIsolatedWorldContentSecurityPolicy(
    int world_id,
    const std::string& security_policy) {
  web_frame_->SetIsolatedWorldContentSecurityPolicy(
      world_id, blink::WebString::FromUTF8(security_policy));
}

void WebFrame::SetIsolatedWorldHumanReadableName(int world_id,
                                                 const std::string& name) {
  web_frame_->SetIsolatedWorldHumanReadableName(
      world_id, blink::WebString::FromUTF8(name));
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

v8::Local<v8::Value> WebFrame::Opener() const {
  blink::WebFrame* frame = web_frame_->Opener();
  if (frame && frame->IsWebLocalFrame())
    return mate::CreateHandle(isolate(),
                              new WebFrame(isolate(), frame->ToWebLocalFrame()))
        .ToV8();
  else
    return v8::Null(isolate());
}

v8::Local<v8::Value> WebFrame::Parent() const {
  blink::WebFrame* frame = web_frame_->Parent();
  if (frame && frame->IsWebLocalFrame())
    return mate::CreateHandle(isolate(),
                              new WebFrame(isolate(), frame->ToWebLocalFrame()))
        .ToV8();
  else
    return v8::Null(isolate());
}

v8::Local<v8::Value> WebFrame::Top() const {
  blink::WebFrame* frame = web_frame_->Top();
  if (frame && frame->IsWebLocalFrame())
    return mate::CreateHandle(isolate(),
                              new WebFrame(isolate(), frame->ToWebLocalFrame()))
        .ToV8();
  else
    return v8::Null(isolate());
}

v8::Local<v8::Value> WebFrame::FirstChild() const {
  blink::WebFrame* frame = web_frame_->FirstChild();
  if (frame && frame->IsWebLocalFrame())
    return mate::CreateHandle(isolate(),
                              new WebFrame(isolate(), frame->ToWebLocalFrame()))
        .ToV8();
  else
    return v8::Null(isolate());
}

v8::Local<v8::Value> WebFrame::NextSibling() const {
  blink::WebFrame* frame = web_frame_->NextSibling();
  if (frame && frame->IsWebLocalFrame())
    return mate::CreateHandle(isolate(),
                              new WebFrame(isolate(), frame->ToWebLocalFrame()))
        .ToV8();
  else
    return v8::Null(isolate());
}

v8::Local<v8::Value> WebFrame::GetFrameForSelector(
    const std::string& selector) const {
  blink::WebElement element = web_frame_->GetDocument().QuerySelector(
      blink::WebString::FromUTF8(selector));
  blink::WebLocalFrame* element_frame =
      blink::WebLocalFrame::FromFrameOwnerElement(element);
  if (element_frame)
    return mate::CreateHandle(isolate(), new WebFrame(isolate(), element_frame))
        .ToV8();
  else
    return v8::Null(isolate());
}

v8::Local<v8::Value> WebFrame::FindFrameByName(const std::string& name) const {
  blink::WebLocalFrame* local_frame =
      web_frame_->FindFrameByName(blink::WebString::FromUTF8(name))
          ->ToWebLocalFrame();
  if (local_frame)
    return mate::CreateHandle(isolate(), new WebFrame(isolate(), local_frame))
        .ToV8();
  else
    return v8::Null(isolate());
}

v8::Local<v8::Value> WebFrame::FindFrameByRoutingId(int routing_id) const {
  content::RenderFrame* render_frame =
      content::RenderFrame::FromRoutingID(routing_id);
  blink::WebLocalFrame* local_frame = nullptr;
  if (render_frame)
    local_frame = render_frame->GetWebFrame();
  if (local_frame)
    return mate::CreateHandle(isolate(), new WebFrame(isolate(), local_frame))
        .ToV8();
  else
    return v8::Null(isolate());
}

v8::Local<v8::Value> WebFrame::RoutingId() const {
  int routing_id = content::RenderFrame::GetRoutingIdForWebFrame(web_frame_);
  return v8::Number::New(isolate(), routing_id);
}

// static
void WebFrame::BuildPrototype(v8::Isolate* isolate,
                              v8::Local<v8::FunctionTemplate> prototype) {
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
      .SetMethod("attachIframeGuest", &WebFrame::AttachIframeGuest)
      .SetMethod("detachGuest", &WebFrame::DetachGuest)
      .SetMethod("setSpellCheckProvider", &WebFrame::SetSpellCheckProvider)
      .SetMethod("registerURLSchemeAsBypassingCSP",
                 &WebFrame::RegisterURLSchemeAsBypassingCSP)
      .SetMethod("registerURLSchemeAsPrivileged",
                 &WebFrame::RegisterURLSchemeAsPrivileged)
      .SetMethod("insertText", &WebFrame::InsertText)
      .SetMethod("insertCSS", &WebFrame::InsertCSS)
      .SetMethod("executeJavaScript", &WebFrame::ExecuteJavaScript)
      .SetMethod("executeJavaScriptInIsolatedWorld",
                 &WebFrame::ExecuteJavaScriptInIsolatedWorld)
      .SetMethod("setIsolatedWorldSecurityOrigin",
                 &WebFrame::SetIsolatedWorldSecurityOrigin)
      .SetMethod("setIsolatedWorldContentSecurityPolicy",
                 &WebFrame::SetIsolatedWorldContentSecurityPolicy)
      .SetMethod("setIsolatedWorldHumanReadableName",
                 &WebFrame::SetIsolatedWorldHumanReadableName)
      .SetMethod("getResourceUsage", &WebFrame::GetResourceUsage)
      .SetMethod("clearCache", &WebFrame::ClearCache)
      .SetMethod("getFrameForSelector", &WebFrame::GetFrameForSelector)
      .SetMethod("findFrameByName", &WebFrame::FindFrameByName)
      .SetProperty("opener", &WebFrame::Opener)
      .SetProperty("parent", &WebFrame::Parent)
      .SetProperty("top", &WebFrame::Top)
      .SetProperty("firstChild", &WebFrame::FirstChild)
      .SetProperty("nextSibling", &WebFrame::NextSibling)
      .SetProperty("routingId", &WebFrame::RoutingId)
      .SetMethod("findFrameByRoutingId", &WebFrame::FindFrameByRoutingId);
}

}  // namespace api

}  // namespace atom

namespace {

using atom::api::WebFrame;

void Initialize(v8::Local<v8::Object> exports,
                v8::Local<v8::Value> unused,
                v8::Local<v8::Context> context,
                void* priv) {
  v8::Isolate* isolate = context->GetIsolate();
  mate::Dictionary dict(isolate, exports);
  dict.Set("webFrame", WebFrame::Create(isolate));
  dict.Set("WebFrame", WebFrame::GetConstructor(isolate)->GetFunction());
}

}  // namespace

NODE_BUILTIN_MODULE_CONTEXT_AWARE(atom_renderer_web_frame, Initialize)
