// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "atom/common/api/api_messages.h"
#include "atom/common/api/event_emitter_caller.h"
#include "atom/common/native_mate_converters/blink_converter.h"
#include "atom/common/native_mate_converters/callback.h"
#include "atom/common/native_mate_converters/gfx_converter.h"
#include "atom/common/native_mate_converters/string16_converter.h"
#include "atom/renderer/api/atom_api_spell_check_client.h"
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

class FrameSetSpellChecker : public content::RenderFrameVisitor {
 public:
  FrameSetSpellChecker(SpellCheckClient* spell_check_client,
                       content::RenderFrame* main_frame)
      : spell_check_client_(spell_check_client), main_frame_(main_frame) {
    content::RenderFrame::ForEach(this);
    main_frame->GetWebFrame()->SetSpellCheckPanelHostClient(spell_check_client);
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

  DISALLOW_COPY_AND_ASSIGN(FrameSetSpellChecker);
};

class SpellCheckerHolder : public content::RenderFrameObserver {
 public:
  // Find existing holder for the |render_frame|.
  static SpellCheckerHolder* FromRenderFrame(
      content::RenderFrame* render_frame) {
    for (auto* holder : instances_) {
      if (holder->render_frame() == render_frame)
        return holder;
    }
    return nullptr;
  }

  SpellCheckerHolder(content::RenderFrame* render_frame,
                     std::unique_ptr<SpellCheckClient> spell_check_client)
      : content::RenderFrameObserver(render_frame),
        spell_check_client_(std::move(spell_check_client)) {
    DCHECK(!FromRenderFrame(render_frame));
    instances_.insert(this);
  }

  ~SpellCheckerHolder() final { instances_.erase(this); }

  void UnsetAndDestroy() {
    FrameSetSpellChecker set_spell_checker(nullptr, render_frame());
    delete this;
  }

  // RenderFrameObserver implementation.
  void OnDestruct() final {
    // Since we delete this in WillReleaseScriptContext, this method is unlikely
    // to be called, but override anyway since I'm not sure if there are some
    // corner cases.
    //
    // Note that while there are two "delete this", it is totally fine as the
    // observer unsubscribes automatically in destructor and the other one won't
    // be called.
    //
    // Also note that we should not call UnsetAndDestroy here, as the render
    // frame is going to be destroyed.
    delete this;
  }

  void WillReleaseScriptContext(v8::Local<v8::Context> context,
                                int world_id) final {
    // Unset spell checker when the script context is going to be released, as
    // the spell check implementation lives there.
    UnsetAndDestroy();
  }

 private:
  static std::set<SpellCheckerHolder*> instances_;

  std::unique_ptr<SpellCheckClient> spell_check_client_;
};

}  // namespace

// static
std::set<SpellCheckerHolder*> SpellCheckerHolder::instances_;

void SetName(v8::Local<v8::Value> window, const std::string& name) {
  GetRenderFrame(window)->GetWebFrame()->SetName(
      blink::WebString::FromUTF8(name));
}

double SetZoomLevel(v8::Local<v8::Value> window, double level) {
  double result = 0.0;
  content::RenderFrame* render_frame = GetRenderFrame(window);
  render_frame->Send(new AtomFrameHostMsg_SetTemporaryZoomLevel(
      render_frame->GetRoutingID(), level, &result));
  return result;
}

double GetZoomLevel(v8::Local<v8::Value> window) {
  double result = 0.0;
  content::RenderFrame* render_frame = GetRenderFrame(window);
  render_frame->Send(
      new AtomFrameHostMsg_GetZoomLevel(render_frame->GetRoutingID(), &result));
  return result;
}

double SetZoomFactor(v8::Local<v8::Value> window, double factor) {
  return blink::WebView::ZoomLevelToZoomFactor(
      SetZoomLevel(window, blink::WebView::ZoomFactorToZoomLevel(factor)));
}

double GetZoomFactor(v8::Local<v8::Value> window) {
  return blink::WebView::ZoomLevelToZoomFactor(GetZoomLevel(window));
}

void SetVisualZoomLevelLimits(v8::Local<v8::Value> window,
                              double min_level,
                              double max_level) {
  blink::WebFrame* web_frame = GetRenderFrame(window)->GetWebFrame();
  web_frame->View()->SetDefaultPageScaleLimits(min_level, max_level);
  web_frame->View()->SetIgnoreViewportTagScaleLimits(true);
}

void SetLayoutZoomLevelLimits(v8::Local<v8::Value> window,
                              double min_level,
                              double max_level) {
  blink::WebFrame* web_frame = GetRenderFrame(window)->GetWebFrame();
  web_frame->View()->ZoomLimitsChanged(min_level, max_level);
}

v8::Local<v8::Value> RegisterEmbedderCustomElement(
    v8::Local<v8::Value> window,
    const base::string16& name,
    v8::Local<v8::Object> options) {
  return GetRenderFrame(window)
      ->GetWebFrame()
      ->GetDocument()
      .RegisterEmbedderCustomElement(blink::WebString::FromUTF16(name),
                                     options);
}

int GetWebFrameId(v8::Local<v8::Value> window,
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

  return render_frame->GetRoutingID();
}

void SetSpellCheckProvider(mate::Arguments* args,
                           v8::Local<v8::Value> window,
                           const std::string& language,
                           bool auto_spell_correct_turned_on,
                           v8::Local<v8::Object> provider) {
  if (!provider->Has(mate::StringToV8(args->isolate(), "spellCheck"))) {
    args->ThrowError("\"spellCheck\" has to be defined");
    return;
  }

  // Remove the old client.
  content::RenderFrame* render_frame = GetRenderFrame(window);
  auto* existing = SpellCheckerHolder::FromRenderFrame(render_frame);
  if (existing)
    existing->UnsetAndDestroy();

  // Set spellchecker for all live frames in the same process or
  // in the sandbox mode for all live sub frames to this WebFrame.
  auto spell_check_client = std::make_unique<SpellCheckClient>(
      language, auto_spell_correct_turned_on, args->isolate(), provider);
  FrameSetSpellChecker spell_checker(spell_check_client.get(), render_frame);

  // Attach the spell checker to RenderFrame.
  new SpellCheckerHolder(render_frame, std::move(spell_check_client));
}

void RegisterURLSchemeAsBypassingCSP(v8::Local<v8::Value> window,
                                     const std::string& scheme) {
  // Register scheme to bypass pages's Content Security Policy.
  blink::SchemeRegistry::RegisterURLSchemeAsBypassingContentSecurityPolicy(
      WTF::String::FromUTF8(scheme.data(), scheme.length()));
}

void RegisterURLSchemeAsPrivileged(v8::Local<v8::Value> window,
                                   const std::string& scheme,
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
  if (args->Length() == 3) {
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

void InsertText(v8::Local<v8::Value> window, const std::string& text) {
  blink::WebFrame* web_frame = GetRenderFrame(window)->GetWebFrame();
  if (web_frame->IsWebLocalFrame()) {
    web_frame->ToWebLocalFrame()
        ->FrameWidget()
        ->GetActiveWebInputMethodController()
        ->CommitText(blink::WebString::FromUTF8(text),
                     blink::WebVector<blink::WebImeTextSpan>(),
                     blink::WebRange(), 0);
  }
}

void InsertCSS(v8::Local<v8::Value> window, const std::string& css) {
  blink::WebFrame* web_frame = GetRenderFrame(window)->GetWebFrame();
  if (web_frame->IsWebLocalFrame()) {
    web_frame->ToWebLocalFrame()->GetDocument().InsertStyleSheet(
        blink::WebString::FromUTF8(css));
  }
}

void ExecuteJavaScript(v8::Local<v8::Value> window,
                       const base::string16& code,
                       mate::Arguments* args) {
  bool has_user_gesture = false;
  args->GetNext(&has_user_gesture);
  ScriptExecutionCallback::CompletionCallback completion_callback;
  args->GetNext(&completion_callback);
  std::unique_ptr<blink::WebScriptExecutionCallback> callback(
      new ScriptExecutionCallback(completion_callback));
  GetRenderFrame(window)->GetWebFrame()->RequestExecuteScriptAndReturnValue(
      blink::WebScriptSource(blink::WebString::FromUTF16(code)),
      has_user_gesture, callback.release());
}

void ExecuteJavaScriptInIsolatedWorld(
    v8::Local<v8::Value> window,
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

  GetRenderFrame(window)->GetWebFrame()->RequestExecuteScriptInIsolatedWorld(
      world_id, &sources.front(), sources.size(), has_user_gesture,
      scriptExecutionType, callback.release());
}

void SetIsolatedWorldSecurityOrigin(v8::Local<v8::Value> window,
                                    int world_id,
                                    const std::string& origin_url) {
  GetRenderFrame(window)->GetWebFrame()->SetIsolatedWorldSecurityOrigin(
      world_id, blink::WebSecurityOrigin::CreateFromString(
                    blink::WebString::FromUTF8(origin_url)));
}

void SetIsolatedWorldContentSecurityPolicy(v8::Local<v8::Value> window,
                                           int world_id,
                                           const std::string& security_policy) {
  GetRenderFrame(window)->GetWebFrame()->SetIsolatedWorldContentSecurityPolicy(
      world_id, blink::WebString::FromUTF8(security_policy));
}

void SetIsolatedWorldHumanReadableName(v8::Local<v8::Value> window,
                                       int world_id,
                                       const std::string& name) {
  GetRenderFrame(window)->GetWebFrame()->SetIsolatedWorldHumanReadableName(
      world_id, blink::WebString::FromUTF8(name));
}

blink::WebCache::ResourceTypeStats GetResourceUsage(v8::Isolate* isolate) {
  blink::WebCache::ResourceTypeStats stats;
  blink::WebCache::GetResourceTypeStats(&stats);
  return stats;
}

void ClearCache(v8::Isolate* isolate) {
  isolate->IdleNotificationDeadline(0.5);
  blink::WebCache::Clear();
  base::MemoryPressureListener::NotifyMemoryPressure(
      base::MemoryPressureListener::MEMORY_PRESSURE_LEVEL_CRITICAL);
}

v8::Local<v8::Value> FindFrameByRoutingId(v8::Isolate* isolate,
                                          v8::Local<v8::Value> window,
                                          int routing_id) {
  content::RenderFrame* render_frame =
      content::RenderFrame::FromRoutingID(routing_id);
  if (render_frame)
    return render_frame->GetWebFrame()->MainWorldScriptContext()->Global();
  else
    return v8::Null(isolate);
}

v8::Local<v8::Value> GetOpener(v8::Isolate* isolate,
                               v8::Local<v8::Value> window) {
  blink::WebFrame* frame = GetRenderFrame(window)->GetWebFrame()->Opener();
  if (frame && frame->IsWebLocalFrame())
    return frame->ToWebLocalFrame()->MainWorldScriptContext()->Global();
  else
    return v8::Null(isolate);
}

v8::Local<v8::Value> GetFrameParent(v8::Isolate* isolate,
                                    v8::Local<v8::Value> window) {
  blink::WebFrame* frame = GetRenderFrame(window)->GetWebFrame()->Parent();
  if (frame && frame->IsWebLocalFrame())
    return frame->ToWebLocalFrame()->MainWorldScriptContext()->Global();
  else
    return v8::Null(isolate);
}

v8::Local<v8::Value> GetTop(v8::Isolate* isolate, v8::Local<v8::Value> window) {
  blink::WebFrame* frame = GetRenderFrame(window)->GetWebFrame()->Top();
  if (frame && frame->IsWebLocalFrame())
    return frame->ToWebLocalFrame()->MainWorldScriptContext()->Global();
  else
    return v8::Null(isolate);
}

v8::Local<v8::Value> GetFirstChild(v8::Isolate* isolate,
                                   v8::Local<v8::Value> window) {
  blink::WebFrame* frame = GetRenderFrame(window)->GetWebFrame()->FirstChild();
  if (frame && frame->IsWebLocalFrame())
    return frame->ToWebLocalFrame()->MainWorldScriptContext()->Global();
  else
    return v8::Null(isolate);
}

v8::Local<v8::Value> GetNextSibling(v8::Isolate* isolate,
                                    v8::Local<v8::Value> window) {
  blink::WebFrame* frame = GetRenderFrame(window)->GetWebFrame()->NextSibling();
  if (frame && frame->IsWebLocalFrame())
    return frame->ToWebLocalFrame()->MainWorldScriptContext()->Global();
  else
    return v8::Null(isolate);
}

v8::Local<v8::Value> GetFrameForSelector(v8::Isolate* isolate,
                                         v8::Local<v8::Value> window,
                                         const std::string& selector) {
  blink::WebElement element =
      GetRenderFrame(window)->GetWebFrame()->GetDocument().QuerySelector(
          blink::WebString::FromUTF8(selector));
  if (element.IsNull())  // not found
    return v8::Null(isolate);

  blink::WebFrame* frame = blink::WebFrame::FromFrameOwnerElement(element);
  if (frame && frame->IsWebLocalFrame())
    return frame->ToWebLocalFrame()->MainWorldScriptContext()->Global();
  else
    return v8::Null(isolate);
}

v8::Local<v8::Value> FindFrameByName(v8::Isolate* isolate,
                                     v8::Local<v8::Value> window,
                                     const std::string& name) {
  blink::WebFrame* frame =
      GetRenderFrame(window)->GetWebFrame()->FindFrameByName(
          blink::WebString::FromUTF8(name));
  if (frame && frame->IsWebLocalFrame())
    return frame->ToWebLocalFrame()->MainWorldScriptContext()->Global();
  else
    return v8::Null(isolate);
}

int GetRoutingId(v8::Local<v8::Value> window) {
  return GetRenderFrame(window)->GetRoutingID();
}

}  // namespace api

}  // namespace atom

namespace {

void Initialize(v8::Local<v8::Object> exports,
                v8::Local<v8::Value> unused,
                v8::Local<v8::Context> context,
                void* priv) {
  using namespace atom::api;  // NOLINT(build/namespaces)

  v8::Isolate* isolate = context->GetIsolate();
  mate::Dictionary dict(isolate, exports);
  dict.SetMethod("setName", &SetName);
  dict.SetMethod("setZoomLevel", &SetZoomLevel);
  dict.SetMethod("getZoomLevel", &GetZoomLevel);
  dict.SetMethod("setZoomFactor", &SetZoomFactor);
  dict.SetMethod("getZoomFactor", &GetZoomFactor);
  dict.SetMethod("setVisualZoomLevelLimits", &SetVisualZoomLevelLimits);
  dict.SetMethod("setLayoutZoomLevelLimits", &SetLayoutZoomLevelLimits);
  dict.SetMethod("registerEmbedderCustomElement",
                 &RegisterEmbedderCustomElement);
  dict.SetMethod("getWebFrameId", &GetWebFrameId);
  dict.SetMethod("setSpellCheckProvider", &SetSpellCheckProvider);
  dict.SetMethod("registerURLSchemeAsBypassingCSP",
                 &RegisterURLSchemeAsBypassingCSP);
  dict.SetMethod("registerURLSchemeAsPrivileged",
                 &RegisterURLSchemeAsPrivileged);
  dict.SetMethod("insertText", &InsertText);
  dict.SetMethod("insertCSS", &InsertCSS);
  dict.SetMethod("executeJavaScript", &ExecuteJavaScript);
  dict.SetMethod("executeJavaScriptInIsolatedWorld",
                 &ExecuteJavaScriptInIsolatedWorld);
  dict.SetMethod("setIsolatedWorldSecurityOrigin",
                 &SetIsolatedWorldSecurityOrigin);
  dict.SetMethod("setIsolatedWorldContentSecurityPolicy",
                 &SetIsolatedWorldContentSecurityPolicy);
  dict.SetMethod("setIsolatedWorldHumanReadableName",
                 &SetIsolatedWorldHumanReadableName);
  dict.SetMethod("getResourceUsage", &GetResourceUsage);
  dict.SetMethod("clearCache", &ClearCache);
  dict.SetMethod("_findFrameByRoutingId", &FindFrameByRoutingId);
  dict.SetMethod("_getFrameForSelector", &GetFrameForSelector);
  dict.SetMethod("_findFrameByName", &FindFrameByName);
  dict.SetMethod("_getOpener", &GetOpener);
  dict.SetMethod("_getParent", &GetFrameParent);
  dict.SetMethod("_getTop", &GetTop);
  dict.SetMethod("_getFirstChild", &GetFirstChild);
  dict.SetMethod("_getNextSibling", &GetNextSibling);
  dict.SetMethod("_getRoutingId", &GetRoutingId);
}

}  // namespace

NODE_BUILTIN_MODULE_CONTEXT_AWARE(atom_renderer_web_frame, Initialize)
