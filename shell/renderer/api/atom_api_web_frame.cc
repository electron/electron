// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/memory/memory_pressure_listener.h"
#include "content/public/renderer/render_frame.h"
#include "content/public/renderer/render_frame_observer.h"
#include "content/public/renderer/render_frame_visitor.h"
#include "content/public/renderer/render_view.h"
#include "services/service_manager/public/cpp/interface_provider.h"
#include "shell/common/api/api.mojom.h"
#include "shell/common/gin_converters/blink_converter.h"
#include "shell/common/gin_helper/dictionary.h"
#include "shell/common/gin_helper/promise.h"
#include "shell/common/node_includes.h"
#include "shell/renderer/api/atom_api_spell_check_client.h"
#include "third_party/blink/public/common/page/page_zoom.h"
#include "third_party/blink/public/common/web_cache/web_cache_resource_type_stats.h"
#include "third_party/blink/public/platform/web_cache.h"
#include "third_party/blink/public/platform/web_isolated_world_info.h"
#include "third_party/blink/public/web/web_custom_element.h"
#include "third_party/blink/public/web/web_document.h"
#include "third_party/blink/public/web/web_element.h"
#include "third_party/blink/public/web/web_frame_widget.h"
#include "third_party/blink/public/web/web_ime_text_span.h"
#include "third_party/blink/public/web/web_input_method_controller.h"
#include "third_party/blink/public/web/web_local_frame.h"
#include "third_party/blink/public/web/web_script_execution_callback.h"
#include "third_party/blink/public/web/web_script_source.h"
#include "third_party/blink/public/web/web_view.h"
#include "url/url_util.h"

namespace gin {

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

template <>
struct Converter<blink::WebDocument::CSSOrigin> {
  static bool FromV8(v8::Isolate* isolate,
                     v8::Local<v8::Value> val,
                     blink::WebDocument::CSSOrigin* out) {
    std::string css_origin;
    if (!ConvertFromV8(isolate, val, &css_origin))
      return false;
    if (css_origin == "user") {
      *out = blink::WebDocument::CSSOrigin::kUserOrigin;
    } else if (css_origin == "author") {
      *out = blink::WebDocument::CSSOrigin::kAuthorOrigin;
    } else {
      return false;
    }
    return true;
  }
};

}  // namespace gin

namespace electron {

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

class RenderFrameStatus final : public content::RenderFrameObserver {
 public:
  explicit RenderFrameStatus(content::RenderFrame* render_frame)
      : content::RenderFrameObserver(render_frame) {}
  ~RenderFrameStatus() final = default;

  bool is_ok() { return render_frame() != nullptr; }

  // RenderFrameObserver implementation.
  void OnDestruct() final {}
};

class ScriptExecutionCallback : public blink::WebScriptExecutionCallback {
 public:
  explicit ScriptExecutionCallback(
      gin_helper::Promise<v8::Local<v8::Value>> promise)
      : promise_(std::move(promise)) {}
  ~ScriptExecutionCallback() override = default;

  void Completed(
      const blink::WebVector<v8::Local<v8::Value>>& result) override {
    if (!result.empty()) {
      if (!result[0].IsEmpty()) {
        // Right now only single results per frame is supported.
        promise_.Resolve(result[0]);
      } else {
        promise_.RejectWithErrorMessage(
            "Script failed to execute, this normally means an error "
            "was thrown. Check the renderer console for the error.");
      }
    } else {
      promise_.RejectWithErrorMessage(
          "WebFrame was removed before script could run. This normally means "
          "the underlying frame was destroyed");
    }
    delete this;
  }

 private:
  gin_helper::Promise<v8::Local<v8::Value>> promise_;

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

class SpellCheckerHolder final : public content::RenderFrameObserver {
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

void SetZoomLevel(v8::Local<v8::Value> window, double level) {
  content::RenderFrame* render_frame = GetRenderFrame(window);
  mojom::ElectronBrowserPtr browser_ptr;
  render_frame->GetRemoteInterfaces()->GetInterface(
      mojo::MakeRequest(&browser_ptr));
  browser_ptr->SetTemporaryZoomLevel(level);
}

double GetZoomLevel(v8::Local<v8::Value> window) {
  double result = 0.0;
  content::RenderFrame* render_frame = GetRenderFrame(window);
  mojom::ElectronBrowserPtr browser_ptr;
  render_frame->GetRemoteInterfaces()->GetInterface(
      mojo::MakeRequest(&browser_ptr));
  browser_ptr->DoGetZoomLevel(&result);
  return result;
}

void SetZoomFactor(v8::Local<v8::Value> window, double factor) {
  SetZoomLevel(window, blink::PageZoomFactorToZoomLevel(factor));
}

double GetZoomFactor(v8::Local<v8::Value> window) {
  return blink::PageZoomLevelToZoomFactor(GetZoomLevel(window));
}

void SetVisualZoomLevelLimits(v8::Local<v8::Value> window,
                              double min_level,
                              double max_level) {
  blink::WebFrame* web_frame = GetRenderFrame(window)->GetWebFrame();
  web_frame->View()->SetDefaultPageScaleLimits(min_level, max_level);
  web_frame->View()->SetIgnoreViewportTagScaleLimits(true);
}

void AllowGuestViewElementDefinition(v8::Isolate* isolate,
                                     v8::Local<v8::Value> window,
                                     v8::Local<v8::Object> context,
                                     v8::Local<v8::Function> register_cb) {
  v8::HandleScope handle_scope(isolate);
  v8::Context::Scope context_scope(context->CreationContext());
  blink::WebCustomElement::EmbedderNamesAllowedScope embedder_names_scope;
  GetRenderFrame(context)->GetWebFrame()->RequestExecuteV8Function(
      context->CreationContext(), register_cb, v8::Null(isolate), 0, nullptr,
      nullptr);
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

void SetSpellCheckProvider(gin_helper::Arguments* args,
                           v8::Local<v8::Value> window,
                           const std::string& language,
                           v8::Local<v8::Object> provider) {
  auto context = args->isolate()->GetCurrentContext();
  if (!provider->Has(context, gin::StringToV8(args->isolate(), "spellCheck"))
           .ToChecked()) {
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
  auto spell_check_client =
      std::make_unique<SpellCheckClient>(language, args->isolate(), provider);
  FrameSetSpellChecker spell_checker(spell_check_client.get(), render_frame);

  // Attach the spell checker to RenderFrame.
  new SpellCheckerHolder(render_frame, std::move(spell_check_client));
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

base::string16 InsertCSS(v8::Local<v8::Value> window,
                         const std::string& css,
                         gin_helper::Arguments* args) {
  blink::WebDocument::CSSOrigin css_origin =
      blink::WebDocument::CSSOrigin::kAuthorOrigin;

  gin_helper::Dictionary options;
  if (args->GetNext(&options))
    options.Get("cssOrigin", &css_origin);

  blink::WebFrame* web_frame = GetRenderFrame(window)->GetWebFrame();
  if (web_frame->IsWebLocalFrame()) {
    return web_frame->ToWebLocalFrame()
        ->GetDocument()
        .InsertStyleSheet(blink::WebString::FromUTF8(css), nullptr, css_origin)
        .Utf16();
  }
  return base::string16();
}

void RemoveInsertedCSS(v8::Local<v8::Value> window, const base::string16& key) {
  blink::WebFrame* web_frame = GetRenderFrame(window)->GetWebFrame();
  if (web_frame->IsWebLocalFrame()) {
    web_frame->ToWebLocalFrame()->GetDocument().RemoveInsertedStyleSheet(
        blink::WebString::FromUTF16(key));
  }
}

v8::Local<v8::Promise> ExecuteJavaScript(gin_helper::Arguments* args,
                                         v8::Local<v8::Value> window,
                                         const base::string16& code) {
  v8::Isolate* isolate = args->isolate();
  gin_helper::Promise<v8::Local<v8::Value>> promise(isolate);
  v8::Local<v8::Promise> handle = promise.GetHandle();

  bool has_user_gesture = false;
  args->GetNext(&has_user_gesture);

  GetRenderFrame(window)->GetWebFrame()->RequestExecuteScriptAndReturnValue(
      blink::WebScriptSource(blink::WebString::FromUTF16(code)),
      has_user_gesture, new ScriptExecutionCallback(std::move(promise)));

  return handle;
}

v8::Local<v8::Promise> ExecuteJavaScriptInIsolatedWorld(
    gin_helper::Arguments* args,
    v8::Local<v8::Value> window,
    int world_id,
    const std::vector<gin_helper::Dictionary>& scripts) {
  v8::Isolate* isolate = args->isolate();
  gin_helper::Promise<v8::Local<v8::Value>> promise(isolate);
  v8::Local<v8::Promise> handle = promise.GetHandle();

  std::vector<blink::WebScriptSource> sources;

  for (const auto& script : scripts) {
    base::string16 code;
    base::string16 url;
    int start_line = 1;
    script.Get("url", &url);
    script.Get("startLine", &start_line);

    if (!script.Get("code", &code)) {
      promise.RejectWithErrorMessage("Invalid 'code'");
      return handle;
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

  // Debugging tip: if you see a crash stack trace beginning from this call,
  // then it is very likely that some exception happened when executing the
  // "content_script/init.js" script.
  GetRenderFrame(window)->GetWebFrame()->RequestExecuteScriptInIsolatedWorld(
      world_id, &sources.front(), sources.size(), has_user_gesture,
      scriptExecutionType, new ScriptExecutionCallback(std::move(promise)));

  return handle;
}

void SetIsolatedWorldInfo(v8::Local<v8::Value> window,
                          int world_id,
                          const gin_helper::Dictionary& options,
                          gin_helper::Arguments* args) {
  std::string origin_url, security_policy, name;
  options.Get("securityOrigin", &origin_url);
  options.Get("csp", &security_policy);
  options.Get("name", &name);

  if (!security_policy.empty() && origin_url.empty()) {
    args->ThrowError(
        "If csp is specified, securityOrigin should also be specified");
    return;
  }

  blink::WebIsolatedWorldInfo info;
  info.security_origin = blink::WebSecurityOrigin::CreateFromString(
      blink::WebString::FromUTF8(origin_url));
  info.content_security_policy = blink::WebString::FromUTF8(security_policy);
  info.human_readable_name = blink::WebString::FromUTF8(name);
  GetRenderFrame(window)->GetWebFrame()->SetIsolatedWorldInfo(world_id, info);
}

blink::WebCacheResourceTypeStats GetResourceUsage(v8::Isolate* isolate) {
  blink::WebCacheResourceTypeStats stats;
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

// Don't name it as GetParent, Windows has API with same name.
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

}  // namespace electron

namespace {

void Initialize(v8::Local<v8::Object> exports,
                v8::Local<v8::Value> unused,
                v8::Local<v8::Context> context,
                void* priv) {
  using namespace electron::api;  // NOLINT(build/namespaces)

  v8::Isolate* isolate = context->GetIsolate();
  gin_helper::Dictionary dict(isolate, exports);
  dict.SetMethod("setName", &SetName);
  dict.SetMethod("setZoomLevel", &SetZoomLevel);
  dict.SetMethod("getZoomLevel", &GetZoomLevel);
  dict.SetMethod("setZoomFactor", &SetZoomFactor);
  dict.SetMethod("getZoomFactor", &GetZoomFactor);
  dict.SetMethod("setVisualZoomLevelLimits", &SetVisualZoomLevelLimits);
  dict.SetMethod("allowGuestViewElementDefinition",
                 &AllowGuestViewElementDefinition);
  dict.SetMethod("getWebFrameId", &GetWebFrameId);
  dict.SetMethod("setSpellCheckProvider", &SetSpellCheckProvider);
  dict.SetMethod("insertText", &InsertText);
  dict.SetMethod("insertCSS", &InsertCSS);
  dict.SetMethod("removeInsertedCSS", &RemoveInsertedCSS);
  dict.SetMethod("executeJavaScript", &ExecuteJavaScript);
  dict.SetMethod("executeJavaScriptInIsolatedWorld",
                 &ExecuteJavaScriptInIsolatedWorld);
  dict.SetMethod("setIsolatedWorldInfo", &SetIsolatedWorldInfo);
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

NODE_LINKED_MODULE_CONTEXT_AWARE(atom_renderer_web_frame, Initialize)
