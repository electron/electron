// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/renderer/extensions/atom_extensions_render_view_observer.h"

#include <vector>
#include "atom/common/api/api_messages.h"
#include "atom/common/native_mate_converters/string16_converter.h"
#include "atom/common/native_mate_converters/value_converter.h"
#include "content/public/renderer/render_view.h"
#include "content/public/renderer/render_view_observer.h"
#include "extensions/renderer/script_context.h"
#include "extensions/renderer/script_context_set.h"
#include "native_mate/dictionary.h"
#include "third_party/WebKit/public/web/WebFrame.h"
#include "third_party/WebKit/public/web/WebKit.h"
#include "third_party/WebKit/public/web/WebLocalFrame.h"
#include "third_party/WebKit/public/web/WebView.h"

namespace extensions {

namespace {
  std::vector<v8::Local<v8::Value>> ListValueToVector(v8::Isolate* isolate,
                                                  const base::ListValue& list) {
    v8::Local<v8::Value> array = mate::ConvertToV8(isolate, list);
    std::vector<v8::Local<v8::Value>> result;
    mate::ConvertFromV8(isolate, array, &result);
    return result;
  }
}  // namespace

AtomExtensionsRenderViewObserver::AtomExtensionsRenderViewObserver(
    content::RenderView* render_view)
    : content::RenderViewObserver(render_view) {}

AtomExtensionsRenderViewObserver::~AtomExtensionsRenderViewObserver() {}

// content::RenderViewObserver implementation.
void AtomExtensionsRenderViewObserver::DidCreateDocumentElement(
                                                blink::WebLocalFrame* frame) {
  v8::Isolate* isolate = blink::mainThreadIsolate();
  v8::HandleScope handle_scope(isolate);

  v8::Local<v8::Context> context = frame->mainWorldScriptContext();
  v8::Context::Scope context_scope(context);

  auto script_context = ScriptContextSet::GetContextByV8Context(context);

  if (script_context->context_type() == Feature::Context::WEB_PAGE_CONTEXT)
    script_context->module_system()
        ->CallModuleMethod("ipc", "didCreateDocumentElement");
}

bool AtomExtensionsRenderViewObserver::OnMessageReceived(
                                                const IPC::Message& message) {
  bool handled = true;
  IPC_BEGIN_MESSAGE_MAP(AtomExtensionsRenderViewObserver, message)
    IPC_MESSAGE_HANDLER(AtomViewMsg_Message, OnBrowserMessage)
    IPC_MESSAGE_UNHANDLED(handled = false)
  IPC_END_MESSAGE_MAP()

  return handled;
}

void AtomExtensionsRenderViewObserver::OnBrowserMessage(
                                                const base::string16& channel,
                                                const base::ListValue& args) {
  if (!render_view()->GetWebView())
    return;

  blink::WebFrame* frame = render_view()->GetWebView()->mainFrame();
  if (!frame || frame->isWebRemoteFrame())
    return;

  v8::Isolate* isolate = blink::mainThreadIsolate();
  v8::HandleScope handle_scope(isolate);

  v8::Local<v8::Context> context = frame->mainWorldScriptContext();
  v8::Context::Scope context_scope(context);

  auto script_context = ScriptContextSet::GetContextByV8Context(context);

  auto args_vector = ListValueToVector(isolate, args);

  // Insert the Event object, event.sender is ipc
  mate::Dictionary event = mate::Dictionary::CreateEmpty(isolate);
  args_vector.insert(args_vector.begin(), event.GetHandle());

  std::vector<v8::Local<v8::Value>> concatenated_args =
        { mate::StringToV8(isolate, channel) };
      concatenated_args.reserve(1 + args_vector.size());
      concatenated_args.insert(concatenated_args.end(),
                                args_vector.begin(), args_vector.end());

  script_context->module_system()
                ->CallModuleMethod("ipc_utils",
                                  "emit",
                                  concatenated_args.size(),
                                  &concatenated_args.front());
}

}  // namespace extensions
