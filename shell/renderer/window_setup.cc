// Copyright (c) 2026 Anthropic, PBC.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/renderer/window_setup.h"

#include "base/command_line.h"
#include "content/public/renderer/render_frame.h"
#include "gin/converter.h"
#include "mojo/public/cpp/bindings/associated_remote.h"
#include "shell/common/options_switches.h"
#include "shell/common/web_contents_utility.mojom.h"
#include "third_party/blink/public/common/associated_interfaces/associated_interface_provider.h"
#include "third_party/blink/public/common/web_preferences/web_preferences.h"
#include "third_party/blink/public/web/web_document.h"
#include "third_party/blink/public/web/web_local_frame.h"
#include "third_party/blink/renderer/bindings/core/v8/v8_binding_for_core.h"  // nogncheck
#include "third_party/blink/renderer/core/dom/events/event.h"  // nogncheck
#include "third_party/blink/renderer/core/dom/events/native_event_listener.h"  // nogncheck
#include "third_party/blink/renderer/core/event_type_names.h"  // nogncheck
#include "third_party/blink/renderer/core/frame/local_dom_window.h"  // nogncheck
#include "third_party/blink/renderer/platform/heap/garbage_collected.h"  // nogncheck
#include "url/gurl.h"
#include "v8/include/v8-context.h"
#include "v8/include/v8-exception.h"
#include "v8/include/v8-function.h"

namespace electron {

namespace {

mojom::ElectronWebContentsUtility* GetUtility(
    content::RenderFrame* render_frame,
    mojo::AssociatedRemote<mojom::ElectronWebContentsUtility>& remote) {
  render_frame->GetRemoteAssociatedInterfaces()->GetInterface(&remote);
  return remote.get();
}

content::RenderFrame* CurrentRenderFrame(v8::Isolate* isolate) {
  blink::WebLocalFrame* frame =
      blink::WebLocalFrame::FrameForContext(isolate->GetCurrentContext());
  return frame ? content::RenderFrame::FromWebFrame(frame) : nullptr;
}

void Prompt(const v8::FunctionCallbackInfo<v8::Value>& info) {
  v8::Isolate* isolate = info.GetIsolate();
  isolate->ThrowException(v8::Exception::Error(
      gin::StringToV8(isolate, "prompt() is not supported.")));
}

void Close(const v8::FunctionCallbackInfo<v8::Value>& info) {
  if (auto* render_frame = CurrentRenderFrame(info.GetIsolate())) {
    mojo::AssociatedRemote<mojom::ElectronWebContentsUtility> utility;
    GetUtility(render_frame, utility)->CloseWindow();
  }
}

void SetFunction(v8::Local<v8::Context> context,
                 const char* name,
                 v8::FunctionCallback callback) {
  v8::Isolate* isolate = v8::Isolate::GetCurrent();
  v8::Local<v8::Function> function;
  if (v8::Function::New(context, callback, {}, 0,
                        v8::ConstructorBehavior::kThrow)
          .ToLocal(&function)) {
    v8::Local<v8::String> key = gin::StringToSymbol(isolate, name);
    function->SetName(key);
    context->Global()->Set(context, key, function).Check();
  }
}

// Reports a <webview> guest's window focus and blur to the browser, which
// forwards them to the embedder's <webview> element. Blink has no frame-level
// focus observer that covers guests, so listen for the DOM events natively.
class GuestFocusListener final : public blink::NativeEventListener {
 public:
  void Invoke(blink::ExecutionContext* execution_context,
              blink::Event* event) override {
    auto* window = blink::DynamicTo<blink::LocalDOMWindow>(execution_context);
    // The window may already be detached from its frame during teardown.
    blink::WebLocalFrame* web_frame =
        window && window->GetFrame()
            ? blink::WebLocalFrame::FromFrameToken(
                  window->GetFrame()->GetLocalFrameToken())
            : nullptr;
    content::RenderFrame* render_frame =
        web_frame ? content::RenderFrame::FromWebFrame(web_frame) : nullptr;
    if (!render_frame)
      return;
    mojo::AssociatedRemote<mojom::ElectronWebContentsUtility> utility;
    GetUtility(render_frame, utility)
        ->NotifyGuestFocusChange(event->type() ==
                                 blink::event_type_names::kFocus);
  }
};

}  // namespace

void SetUpWindow(content::RenderFrame* render_frame,
                 v8::Local<v8::Context> context) {
  blink::WebLocalFrame* web_frame = render_frame->GetWebFrame();
  GURL url(web_frame->GetDocument().Url());
  if (url.SchemeIs("devtools") || url.SchemeIs("chrome") ||
      url.SchemeIs("chrome-extension")) {
    return;
  }
  v8::Context::Scope context_scope(context);

  SetFunction(context, "prompt", Prompt);

  // A popup opened from a <webview> inherits is_webview but is not one.
  const bool is_webview =
      render_frame->GetBlinkPreferences().is_webview && !web_frame->Opener();
  const bool is_main_world = context == web_frame->MainWorldScriptContext();

  // Only the top-level document of a Node.js renderer; frames and sandboxed
  // renderers keep Blink's behaviour.
  const bool sandboxed = base::CommandLine::ForCurrentProcess()->HasSwitch(
      switches::kEnableSandbox);
  if (!sandboxed && render_frame->IsMainFrame() && !is_webview)
    SetFunction(context, "close", Close);

  if (is_webview && render_frame->IsMainFrame() && is_main_world) {
    if (blink::LocalDOMWindow* window = blink::ToLocalDOMWindow(context)) {
      if (!window->GetFrame())
        return;
      auto* listener = blink::MakeGarbageCollected<GuestFocusListener>();
      window->addEventListener(blink::event_type_names::kFocus, listener);
      window->addEventListener(blink::event_type_names::kBlur, listener);
    }
  }
}

}  // namespace electron
