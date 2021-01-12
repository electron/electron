// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include <limits>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/command_line.h"
#include "base/memory/memory_pressure_listener.h"
#include "base/strings/utf_string_conversions.h"
#include "components/spellcheck/renderer/spellcheck.h"
#include "content/public/renderer/render_frame.h"
#include "content/public/renderer/render_frame_observer.h"
#include "content/public/renderer/render_frame_visitor.h"
#include "content/public/renderer/render_view.h"
#include "services/service_manager/public/cpp/interface_provider.h"
#include "shell/common/api/api.mojom.h"
#include "shell/common/gin_converters/blink_converter.h"
#include "shell/common/gin_converters/callback_converter.h"
#include "shell/common/gin_converters/file_path_converter.h"
#include "shell/common/gin_helper/dictionary.h"
#include "shell/common/gin_helper/error_thrower.h"
#include "shell/common/gin_helper/promise.h"
#include "shell/common/node_includes.h"
#include "shell/common/options_switches.h"
#include "shell/renderer/api/electron_api_spell_check_client.h"
#include "shell/renderer/electron_renderer_client.h"
#include "third_party/blink/public/common/browser_interface_broker_proxy.h"
#include "third_party/blink/public/common/page/page_zoom.h"
#include "third_party/blink/public/common/web_cache/web_cache_resource_type_stats.h"
#include "third_party/blink/public/common/web_preferences/web_preferences.h"
#include "third_party/blink/public/platform/web_cache.h"
#include "third_party/blink/public/platform/web_isolated_world_info.h"
#include "third_party/blink/public/web/web_custom_element.h"
#include "third_party/blink/public/web/web_document.h"
#include "third_party/blink/public/web/web_element.h"
#include "third_party/blink/public/web/web_frame_widget.h"
#include "third_party/blink/public/web/web_input_method_controller.h"
#include "third_party/blink/public/web/web_local_frame.h"
#include "third_party/blink/public/web/web_script_execution_callback.h"
#include "third_party/blink/public/web/web_script_source.h"
#include "third_party/blink/public/web/web_view.h"
#include "ui/base/ime/ime_text_span.h"
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

#if BUILDFLAG(ENABLE_BUILTIN_SPELLCHECKER)

bool SpellCheckWord(v8::Isolate* isolate,
                    v8::Local<v8::Value> window,
                    const std::string& word,
                    std::vector<base::string16>* optional_suggestions) {
  size_t start;
  size_t length;

  ElectronRendererClient* client = ElectronRendererClient::Get();
  auto* render_frame = GetRenderFrame(window);
  if (!render_frame)
    return true;

  base::string16 w = base::UTF8ToUTF16(word);
  int id = render_frame->GetRoutingID();
  return client->GetSpellCheck()->SpellCheckWord(
      w.c_str(), 0, word.size(), id, &start, &length, optional_suggestions);
}

#endif

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
  // for compatibility with the older version of this, error is after result
  using CompletionCallback =
      base::OnceCallback<void(const v8::Local<v8::Value>& result,
                              const v8::Local<v8::Value>& error)>;

  explicit ScriptExecutionCallback(
      gin_helper::Promise<v8::Local<v8::Value>> promise,
      bool world_safe_result,
      CompletionCallback callback)
      : promise_(std::move(promise)),
        world_safe_result_(world_safe_result),
        callback_(std::move(callback)) {}

  ~ScriptExecutionCallback() override = default;

  void CopyResultToCallingContextAndFinalize(
      v8::Isolate* isolate,
      const v8::Local<v8::Value>& result) {
    blink::CloneableMessage ret;
    bool success;
    std::string error_message;
    {
      v8::TryCatch try_catch(isolate);
      success = gin::ConvertFromV8(isolate, result, &ret);
      if (try_catch.HasCaught()) {
        auto message = try_catch.Message();

        if (message.IsEmpty() ||
            !gin::ConvertFromV8(isolate, message->Get(), &error_message)) {
          error_message =
              "An unknown exception occurred while getting the result of "
              "the script";
        }
      }
    }
    if (!success) {
      // Failed convert so we send undefined everywhere
      if (callback_)
        std::move(callback_).Run(
            v8::Undefined(isolate),
            v8::Exception::Error(
                v8::String::NewFromUtf8(isolate, error_message.c_str())
                    .ToLocalChecked()));
      promise_.RejectWithErrorMessage(error_message);
    } else {
      v8::Local<v8::Context> context = promise_.GetContext();
      v8::Context::Scope context_scope(context);
      v8::Local<v8::Value> cloned_value = gin::ConvertToV8(isolate, ret);
      if (callback_)
        std::move(callback_).Run(cloned_value, v8::Undefined(isolate));
      promise_.Resolve(cloned_value);
    }
  }

  void Completed(
      const blink::WebVector<v8::Local<v8::Value>>& result) override {
    v8::Isolate* isolate = v8::Isolate::GetCurrent();
    if (!result.empty()) {
      if (!result[0].IsEmpty()) {
        v8::Local<v8::Value> value = result[0];
        // Either world safe results are disabled or the result was created in
        // the same world as the caller or the result is not an object and
        // therefore does not have a prototype chain to protect
        bool should_clone_value =
            world_safe_result_ &&
            !(value->IsObject() &&
              promise_.GetContext() ==
                  value.As<v8::Object>()->CreationContext()) &&
            value->IsObject();
        if (should_clone_value) {
          CopyResultToCallingContextAndFinalize(isolate, value);
        } else {
          // Right now only single results per frame is supported.
          if (callback_)
            std::move(callback_).Run(value, v8::Undefined(isolate));
          promise_.Resolve(value);
        }
      } else {
        const char* error_message =
            "Script failed to execute, this normally means an error "
            "was thrown. Check the renderer console for the error.";
        if (!callback_.is_null()) {
          v8::Local<v8::Context> context = promise_.GetContext();
          v8::Context::Scope context_scope(context);
          std::move(callback_).Run(
              v8::Undefined(isolate),
              v8::Exception::Error(
                  v8::String::NewFromUtf8(isolate, error_message)
                      .ToLocalChecked()));
        }
        promise_.RejectWithErrorMessage(error_message);
      }
    } else {
      const char* error_message =
          "WebFrame was removed before script could run. This normally means "
          "the underlying frame was destroyed";
      if (!callback_.is_null()) {
        v8::Local<v8::Context> context = promise_.GetContext();
        v8::Context::Scope context_scope(context);
        std::move(callback_).Run(
            v8::Undefined(isolate),
            v8::Exception::Error(v8::String::NewFromUtf8(isolate, error_message)
                                     .ToLocalChecked()));
      }
      promise_.RejectWithErrorMessage(error_message);
    }
    delete this;
  }

 private:
  gin_helper::Promise<v8::Local<v8::Value>> promise_;
  bool world_safe_result_;
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

void SetZoomLevel(gin_helper::ErrorThrower thrower,
                  v8::Local<v8::Value> window,
                  double level) {
  content::RenderFrame* render_frame = GetRenderFrame(window);
  if (!render_frame) {
    thrower.ThrowError(
        "Render frame was torn down before webFrame.setZoomLevel could be "
        "executed");
    return;
  }

  mojo::Remote<mojom::ElectronBrowser> browser_remote;
  render_frame->GetBrowserInterfaceBroker()->GetInterface(
      browser_remote.BindNewPipeAndPassReceiver());
  browser_remote->SetTemporaryZoomLevel(level);
}

double GetZoomLevel(gin_helper::ErrorThrower thrower,
                    v8::Local<v8::Value> window) {
  double result = 0.0;
  content::RenderFrame* render_frame = GetRenderFrame(window);
  if (!render_frame) {
    thrower.ThrowError(
        "Render frame was torn down before webFrame.getZoomLevel could be "
        "executed");
    return result;
  }

  mojo::Remote<mojom::ElectronBrowser> browser_remote;
  render_frame->GetBrowserInterfaceBroker()->GetInterface(
      browser_remote.BindNewPipeAndPassReceiver());
  browser_remote->DoGetZoomLevel(&result);
  return result;
}

void SetZoomFactor(gin_helper::ErrorThrower thrower,
                   v8::Local<v8::Value> window,
                   double factor) {
  if (factor < std::numeric_limits<double>::epsilon()) {
    thrower.ThrowError("'zoomFactor' must be a double greater than 0.0");
    return;
  }

  SetZoomLevel(thrower, window, blink::PageZoomFactorToZoomLevel(factor));
}

double GetZoomFactor(gin_helper::ErrorThrower thrower,
                     v8::Local<v8::Value> window) {
  double zoom_level = GetZoomLevel(thrower, window);
  return blink::PageZoomLevelToZoomFactor(zoom_level);
}

v8::Local<v8::Value> GetWebPreference(v8::Isolate* isolate,
                                      v8::Local<v8::Value> window,
                                      std::string pref_name) {
  content::RenderFrame* render_frame = GetRenderFrame(window);
  const auto& prefs = render_frame->GetBlinkPreferences();

  if (pref_name == options::kPreloadScripts) {
    return gin::ConvertToV8(isolate, prefs.preloads);
  } else if (pref_name == options::kDisableElectronSiteInstanceOverrides) {
    return gin::ConvertToV8(isolate,
                            prefs.disable_electron_site_instance_overrides);
  } else if (pref_name == options::kBackgroundColor) {
    return gin::ConvertToV8(isolate, prefs.background_color);
  } else if (pref_name == options::kOpenerID) {
    // NOTE: openerId is internal-only.
    return gin::ConvertToV8(isolate, prefs.opener_id);
  } else if (pref_name == options::kContextIsolation) {
    return gin::ConvertToV8(isolate, prefs.context_isolation);
#if BUILDFLAG(ENABLE_REMOTE_MODULE)
  } else if (pref_name == options::kEnableRemoteModule) {
    return gin::ConvertToV8(isolate, prefs.enable_remote_module);
#endif
  } else if (pref_name == options::kWorldSafeExecuteJavaScript) {
    return gin::ConvertToV8(isolate, prefs.world_safe_execute_javascript);
  } else if (pref_name == options::kGuestInstanceID) {
    // NOTE: guestInstanceId is internal-only.
    return gin::ConvertToV8(isolate, prefs.guest_instance_id);
  } else if (pref_name == options::kHiddenPage) {
    // NOTE: hiddenPage is internal-only.
    return gin::ConvertToV8(isolate, prefs.hidden_page);
  } else if (pref_name == options::kOffscreen) {
    return gin::ConvertToV8(isolate, prefs.offscreen);
  } else if (pref_name == options::kPreloadScript) {
    return gin::ConvertToV8(isolate, prefs.preload.value());
  } else if (pref_name == options::kNativeWindowOpen) {
    return gin::ConvertToV8(isolate, prefs.native_window_open);
  } else if (pref_name == options::kNodeIntegration) {
    return gin::ConvertToV8(isolate, prefs.node_integration);
  } else if (pref_name == options::kNodeIntegrationInWorker) {
    return gin::ConvertToV8(isolate, prefs.node_integration_in_worker);
  } else if (pref_name == options::kEnableNodeLeakageInRenderers) {
    // NOTE: enableNodeLeakageInRenderers is internal-only.
    return gin::ConvertToV8(isolate, prefs.node_leakage_in_renderers);
  } else if (pref_name == options::kNodeIntegrationInSubFrames) {
    return gin::ConvertToV8(isolate, true);
#if BUILDFLAG(ENABLE_BUILTIN_SPELLCHECKER)
  } else if (pref_name == options::kSpellcheck) {
    return gin::ConvertToV8(isolate, prefs.enable_spellcheck);
#endif
  } else if (pref_name == options::kPlugins) {
    return gin::ConvertToV8(isolate, prefs.enable_plugins);
  } else if (pref_name == options::kEnableWebSQL) {
    return gin::ConvertToV8(isolate, prefs.enable_websql);
  } else if (pref_name == options::kWebviewTag) {
    return gin::ConvertToV8(isolate, prefs.webview_tag);
  }
  return v8::Null(isolate);
}

void SetVisualZoomLevelLimits(gin_helper::ErrorThrower thrower,
                              v8::Local<v8::Value> window,
                              double min_level,
                              double max_level) {
  auto* render_frame = GetRenderFrame(window);
  if (!render_frame) {
    thrower.ThrowError(
        "Render frame was torn down before webFrame.setVisualZoomLevelLimits "
        "could be executed");
    return;
  }

  blink::WebFrame* web_frame = render_frame->GetWebFrame();
  web_frame->View()->SetDefaultPageScaleLimits(min_level, max_level);
}

void AllowGuestViewElementDefinition(gin_helper::ErrorThrower thrower,
                                     v8::Local<v8::Value> window,
                                     v8::Local<v8::Object> context,
                                     v8::Local<v8::Function> register_cb) {
  v8::HandleScope handle_scope(thrower.isolate());
  v8::Context::Scope context_scope(context->CreationContext());
  blink::WebCustomElement::EmbedderNamesAllowedScope embedder_names_scope;

  auto* render_frame = GetRenderFrame(window);
  if (!render_frame) {
    thrower.ThrowError(
        "Render frame was torn down before "
        "webFrame.allowGuestViewElementDefinition could be executed");
    return;
  }

  render_frame->GetWebFrame()->RequestExecuteV8Function(
      context->CreationContext(), register_cb, v8::Null(thrower.isolate()), 0,
      nullptr, nullptr);
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
  if (!render_frame) {
    args->ThrowError(
        "Render frame was torn down before webFrame.setSpellCheckProvider "
        "could be executed");
    return;
  }

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

void InsertText(gin_helper::ErrorThrower thrower,
                v8::Local<v8::Value> window,
                const std::string& text) {
  auto* render_frame = GetRenderFrame(window);
  if (!render_frame) {
    thrower.ThrowError(
        "Render frame was torn down before webFrame.insertText could be "
        "executed");
    return;
  }

  blink::WebFrame* web_frame = render_frame->GetWebFrame();
  if (web_frame->IsWebLocalFrame()) {
    web_frame->ToWebLocalFrame()
        ->FrameWidget()
        ->GetActiveWebInputMethodController()
        ->CommitText(blink::WebString::FromUTF8(text),
                     blink::WebVector<ui::ImeTextSpan>(), blink::WebRange(), 0);
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

  auto* render_frame = GetRenderFrame(window);
  if (!render_frame) {
    args->ThrowError(
        "Render frame was torn down before webFrame.insertCSS could be "
        "executed");
    return base::string16();
  }

  blink::WebFrame* web_frame = render_frame->GetWebFrame();
  if (web_frame->IsWebLocalFrame()) {
    return web_frame->ToWebLocalFrame()
        ->GetDocument()
        .InsertStyleSheet(blink::WebString::FromUTF8(css), nullptr, css_origin)
        .Utf16();
  }
  return base::string16();
}

void RemoveInsertedCSS(gin_helper::ErrorThrower thrower,
                       v8::Local<v8::Value> window,
                       const base::string16& key) {
  auto* render_frame = GetRenderFrame(window);
  if (!render_frame) {
    thrower.ThrowError(
        "Render frame was torn down before webFrame.removeInsertedCSS could be "
        "executed");
    return;
  }

  blink::WebFrame* web_frame = render_frame->GetWebFrame();
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

  auto* render_frame = GetRenderFrame(window);
  if (!render_frame) {
    promise.RejectWithErrorMessage(
        "Render frame was torn down before webFrame.executeJavaScript could be "
        "executed");
    return handle;
  }

  bool has_user_gesture = false;
  args->GetNext(&has_user_gesture);

  ScriptExecutionCallback::CompletionCallback completion_callback;
  args->GetNext(&completion_callback);

  auto& prefs = render_frame->GetBlinkPreferences();

  render_frame->GetWebFrame()->RequestExecuteScriptAndReturnValue(
      blink::WebScriptSource(blink::WebString::FromUTF16(code)),
      has_user_gesture,
      new ScriptExecutionCallback(std::move(promise),
                                  prefs.world_safe_execute_javascript,
                                  std::move(completion_callback)));

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

  auto* render_frame = GetRenderFrame(window);
  if (!render_frame) {
    promise.RejectWithErrorMessage(
        "Render frame was torn down before "
        "webFrame.executeJavaScriptInIsolatedWorld could be executed");
    return handle;
  }

  bool has_user_gesture = false;
  args->GetNext(&has_user_gesture);

  blink::WebLocalFrame::ScriptExecutionType scriptExecutionType =
      blink::WebLocalFrame::kSynchronous;
  args->GetNext(&scriptExecutionType);

  ScriptExecutionCallback::CompletionCallback completion_callback;
  args->GetNext(&completion_callback);

  std::vector<blink::WebScriptSource> sources;

  for (const auto& script : scripts) {
    base::string16 code;
    base::string16 url;
    int start_line = 1;
    script.Get("url", &url);
    script.Get("startLine", &start_line);

    if (!script.Get("code", &code)) {
      const char* error_message = "Invalid 'code'";
      if (!completion_callback.is_null()) {
        std::move(completion_callback)
            .Run(v8::Undefined(isolate),
                 v8::Exception::Error(
                     v8::String::NewFromUtf8(isolate, error_message)
                         .ToLocalChecked()));
      }
      promise.RejectWithErrorMessage(error_message);
      return handle;
    }

    sources.emplace_back(
        blink::WebScriptSource(blink::WebString::FromUTF16(code),
                               blink::WebURL(GURL(url)), start_line));
  }

  auto& prefs = render_frame->GetBlinkPreferences();

  // Debugging tip: if you see a crash stack trace beginning from this call,
  // then it is very likely that some exception happened when executing the
  // "content_script/init.js" script.
  render_frame->GetWebFrame()->RequestExecuteScriptInIsolatedWorld(
      world_id, &sources.front(), sources.size(), has_user_gesture,
      scriptExecutionType,
      new ScriptExecutionCallback(std::move(promise),
                                  prefs.world_safe_execute_javascript,
                                  std::move(completion_callback)));

  return handle;
}

void SetIsolatedWorldInfo(v8::Local<v8::Value> window,
                          int world_id,
                          const gin_helper::Dictionary& options,
                          gin_helper::Arguments* args) {
  auto* render_frame = GetRenderFrame(window);
  if (!render_frame) {
    args->ThrowError(
        "Render frame was torn down before webFrame.setIsolatedWorldInfo could "
        "be executed");
    return;
  }

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
  blink::SetIsolatedWorldInfo(world_id, info);
}

blink::WebCacheResourceTypeStats GetResourceUsage(v8::Isolate* isolate) {
  blink::WebCacheResourceTypeStats stats;
  blink::WebCache::GetResourceTypeStats(&stats);
  return stats;
}

#if BUILDFLAG(ENABLE_BUILTIN_SPELLCHECKER)

bool IsWordMisspelled(v8::Isolate* isolate,
                      v8::Local<v8::Value> window,
                      const std::string& word) {
  return !SpellCheckWord(isolate, window, word, nullptr);
}

std::vector<base::string16> GetWordSuggestions(v8::Isolate* isolate,
                                               v8::Local<v8::Value> window,
                                               const std::string& word) {
  std::vector<base::string16> suggestions;
  SpellCheckWord(isolate, window, word, &suggestions);
  return suggestions;
}

#endif

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
  auto* render_frame = GetRenderFrame(window);
  if (!render_frame)
    return v8::Null(isolate);

  blink::WebFrame* frame = render_frame->GetWebFrame()->Opener();
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
  auto* render_frame = GetRenderFrame(window);
  if (!render_frame)
    return v8::Null(isolate);

  blink::WebFrame* frame = render_frame->GetWebFrame()->Top();
  if (frame && frame->IsWebLocalFrame())
    return frame->ToWebLocalFrame()->MainWorldScriptContext()->Global();
  else
    return v8::Null(isolate);
}

v8::Local<v8::Value> GetFirstChild(v8::Isolate* isolate,
                                   v8::Local<v8::Value> window) {
  auto* render_frame = GetRenderFrame(window);
  if (!render_frame)
    return v8::Null(isolate);

  blink::WebFrame* frame = render_frame->GetWebFrame()->FirstChild();
  if (frame && frame->IsWebLocalFrame())
    return frame->ToWebLocalFrame()->MainWorldScriptContext()->Global();
  else
    return v8::Null(isolate);
}

v8::Local<v8::Value> GetNextSibling(v8::Isolate* isolate,
                                    v8::Local<v8::Value> window) {
  auto* render_frame = GetRenderFrame(window);
  if (!render_frame)
    return v8::Null(isolate);

  blink::WebFrame* frame = render_frame->GetWebFrame()->NextSibling();
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
  auto* render_frame = GetRenderFrame(window);
  if (!render_frame)
    return v8::Null(isolate);

  blink::WebFrame* frame = render_frame->GetWebFrame()->FindFrameByName(
      blink::WebString::FromUTF8(name));
  if (frame && frame->IsWebLocalFrame())
    return frame->ToWebLocalFrame()->MainWorldScriptContext()->Global();
  else
    return v8::Null(isolate);
}

int GetRoutingId(gin_helper::ErrorThrower thrower,
                 v8::Local<v8::Value> window) {
  auto* render_frame = GetRenderFrame(window);
  if (!render_frame) {
    thrower.ThrowError(
        "Render frame was torn down before webFrame.getRoutingId could be "
        "executed");
    return 0;
  }

  return render_frame->GetRoutingID();
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
  dict.SetMethod("getWebPreference", &GetWebPreference);
  dict.SetMethod("setSpellCheckProvider", &SetSpellCheckProvider);
  dict.SetMethod("insertText", &InsertText);
  dict.SetMethod("insertCSS", &InsertCSS);
  dict.SetMethod("removeInsertedCSS", &RemoveInsertedCSS);
  dict.SetMethod("executeJavaScript", &ExecuteJavaScript);
  dict.SetMethod("executeJavaScriptInIsolatedWorld",
                 &ExecuteJavaScriptInIsolatedWorld);
  dict.SetMethod("setIsolatedWorldInfo", &SetIsolatedWorldInfo);
  dict.SetMethod("getResourceUsage", &GetResourceUsage);
#if BUILDFLAG(ENABLE_BUILTIN_SPELLCHECKER)
  dict.SetMethod("isWordMisspelled", &IsWordMisspelled);
  dict.SetMethod("getWordSuggestions", &GetWordSuggestions);
#endif
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

NODE_LINKED_MODULE_CONTEXT_AWARE(electron_renderer_web_frame, Initialize)
