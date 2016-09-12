// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/common/javascript_bindings.h"

#include <vector>
#include "atom/browser/web_contents_preferences.h"
#include "atom/common/api/api_messages.h"
#include "atom/common/native_mate_converters/string16_converter.h"
#include "atom/common/native_mate_converters/value_converter.h"
#include "content/public/renderer/render_frame.h"
#include "content/public/renderer/render_view.h"
#include "extensions/renderer/console.h"
#include "native_mate/dictionary.h"
#include "third_party/WebKit/public/web/WebView.h"
#include "third_party/WebKit/public/web/WebLocalFrame.h"

namespace atom {

namespace {

std::vector<v8::Local<v8::Value>> ListValueToVector(v8::Isolate* isolate,
                                                const base::ListValue& list) {
  v8::Local<v8::Value> array = mate::ConvertToV8(isolate, list);
  std::vector<v8::Local<v8::Value>> result;
  mate::ConvertFromV8(isolate, array, &result);
  return result;
}

}  // namespace

JavascriptBindings::JavascriptBindings(content::RenderView* render_view,
                                       extensions::ScriptContext* context)
    : content::RenderViewObserver(render_view),
      extensions::ObjectBackedNativeHandler(context) {
  RouteFunction(
      "GetBinding",
      base::Bind(&JavascriptBindings::GetBinding, base::Unretained(this)));
}

JavascriptBindings::~JavascriptBindings() {
  Observe(nullptr);
}

void JavascriptBindings::OnDestruct() {
  Observe(nullptr);
}

v8::Local<v8::Value> JavascriptBindings::GetHiddenValue(v8::Isolate* isolate,
                                    v8::Local<v8::String> key) {
  if (!is_valid() || !render_view())
    return v8::Local<v8::Value>();

  v8::Local<v8::Context> main_context =
      render_view()->GetWebView()->mainFrame()->mainWorldScriptContext();
  v8::Local<v8::Private> privateKey = v8::Private::ForApi(isolate, key);
  v8::Local<v8::Value> value;
  v8::Local<v8::Object> object = main_context->Global();

  if (!ContextCanAccessObject(main_context, main_context->Global(), false)) {
    extensions::console::Warn(context()->GetRenderFrame(),
      "cannot access global in main frame script context");
    return v8::Local<v8::Value>();
  }

  v8::Maybe<bool> result = object->HasPrivate(main_context, privateKey);
  if (!(result.IsJust() && result.FromJust()))
    return v8::Local<v8::Value>();
  if (object->GetPrivate(main_context, privateKey).ToLocal(&value))
    return value;
  return v8::Local<v8::Value>();
}

void JavascriptBindings::SetHiddenValue(v8::Isolate* isolate,
                    v8::Local<v8::String> key,
                    v8::Local<v8::Value> value) {
  if (!is_valid() || !render_view() || value.IsEmpty())
    return;

  v8::Local<v8::Context> main_context =
      render_view()->GetWebView()->mainFrame()->mainWorldScriptContext();

  if (!ContextCanAccessObject(main_context, main_context->Global(), false)) {
    extensions::console::Warn(context()->GetRenderFrame(),
          "cannot access global in main frame script context");
    return;
  }

  v8::Local<v8::Private> privateKey = v8::Private::ForApi(isolate, key);
  main_context->Global()->SetPrivate(main_context, privateKey, value);
}

void JavascriptBindings::IPCSend(mate::Arguments* args,
          const base::string16& channel,
          const base::ListValue& arguments) {
  if (!is_valid() || !render_view())
    return;

  bool success = render_view()->Send(new AtomViewHostMsg_Message(
      render_view()->GetRoutingID(), channel, arguments));

  if (!success)
    args->ThrowError("Unable to send AtomViewHostMsg_Message");
}

base::string16 JavascriptBindings::IPCSendSync(mate::Arguments* args,
                        const base::string16& channel,
                        const base::ListValue& arguments) {
  base::string16 json;

  if (!is_valid() || !render_view()) {
    return json;
  }

  IPC::SyncMessage* message = new AtomViewHostMsg_Message_Sync(
      render_view()->GetRoutingID(), channel, arguments, &json);
  bool success = render_view()->Send(message);

  if (!success)
    args->ThrowError("Unable to send AtomViewHostMsg_Message_Sync");

  return json;
}

void JavascriptBindings::GetBinding(
      const v8::FunctionCallbackInfo<v8::Value>& args) {
  blink::WebLocalFrame* frame = context()->web_frame();
  DCHECK(frame);

  v8::Isolate* isolate = args.GetIsolate();
  mate::Dictionary binding(isolate, v8::Object::New(isolate));

  mate::Dictionary ipc(isolate, v8::Object::New(isolate));
  ipc.SetMethod("send", base::Bind(&JavascriptBindings::IPCSend,
      base::Unretained(this)));
  ipc.SetMethod("sendSync", base::Bind(&JavascriptBindings::IPCSendSync,
      base::Unretained(this)));
  binding.Set("ipc", ipc.GetHandle());

  mate::Dictionary v8(isolate, v8::Object::New(isolate));
  v8.SetMethod("getHiddenValue", base::Bind(&JavascriptBindings::GetHiddenValue,
      base::Unretained(this)));
  v8.SetMethod("setHiddenValue", base::Bind(&JavascriptBindings::SetHiddenValue,
      base::Unretained(this)));
  binding.Set("v8", v8.GetHandle());

  args.GetReturnValue().Set(binding.GetHandle());
}

bool JavascriptBindings::OnMessageReceived(const IPC::Message& message) {
  if (!is_valid() || WebContentsPreferences::run_node())
    return false;

  // only handle ipc messages in the main frame script context
  v8::Isolate* isolate = context()->isolate();
  v8::HandleScope handle_scope(isolate);
  if (context()->context_type() !=
        extensions::Feature::BLESSED_EXTENSION_CONTEXT &&
      context()->context_type() !=
        extensions::Feature::UNBLESSED_EXTENSION_CONTEXT &&
      render_view()->GetWebView()->mainFrame()->mainWorldScriptContext() !=
      context()->v8_context())
    return false;

  bool handled = true;
  IPC_BEGIN_MESSAGE_MAP(JavascriptBindings, message)
    IPC_MESSAGE_HANDLER(AtomViewMsg_Message, OnBrowserMessage)
    IPC_MESSAGE_UNHANDLED(handled = false)
  IPC_END_MESSAGE_MAP()

  return handled;
}

void JavascriptBindings::OnBrowserMessage(bool all_frames,
                                          const base::string16& channel,
                                          const base::ListValue& args) {
  if (!is_valid())
    return;

  v8::Isolate* isolate = context()->isolate();
  v8::HandleScope handle_scope(isolate);
  v8::Context::Scope context_scope(context()->v8_context());

  auto args_vector = ListValueToVector(isolate, args);

  // Insert the Event object, event.sender is ipc
  mate::Dictionary event = mate::Dictionary::CreateEmpty(isolate);
  args_vector.insert(args_vector.begin(), event.GetHandle());

  std::vector<v8::Local<v8::Value>> concatenated_args =
        { mate::StringToV8(isolate, channel) };
      concatenated_args.reserve(1 + args_vector.size());
      concatenated_args.insert(concatenated_args.end(),
                                args_vector.begin(), args_vector.end());

  context()->module_system()->CallModuleMethod("ipc_utils",
                                  "emit",
                                  concatenated_args.size(),
                                  &concatenated_args.front());
}

}  // namespace atom
