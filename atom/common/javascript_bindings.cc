// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/common/javascript_bindings.h"

#include "atom/common/api/api_messages.h"
#include "atom/common/native_mate_converters/string16_converter.h"
#include "atom/common/native_mate_converters/value_converter.h"
#include "atom/renderer/api/atom_api_web_frame.h"
#include "content/public/renderer/render_frame.h"
#include "content/public/renderer/render_view.h"
#include "extensions/renderer/script_context.h"
#include "native_mate/dictionary.h"
#include "third_party/WebKit/public/web/WebKit.h"
#include "third_party/WebKit/public/web/WebLocalFrame.h"

namespace {
  // TODO(bridiver) This is copied from atom_api_renderer_ipc.cc
  // and should be cleaned up
  v8::Local<v8::Value> GetHiddenValue(v8::Local<v8::Object> object,
                                      v8::Local<v8::String> key) {
    return object->GetHiddenValue(key);
  }

  void SetHiddenValue(v8::Local<v8::Object> object,
                      v8::Local<v8::String> key,
                      v8::Local<v8::Value> value) {
    object->SetHiddenValue(key, value);
  }

  content::RenderView* GetCurrentRenderView() {
    blink::WebLocalFrame* frame =
      blink::WebLocalFrame::frameForCurrentContext();
    if (!frame)
      return NULL;

    blink::WebView* view = frame->view();
    if (!view)
      return NULL;  // can happen during closing.

    return content::RenderView::FromWebView(view);
  }

  void IPCSend(mate::Arguments* args,
            const base::string16& channel,
            const base::ListValue& arguments) {
    content::RenderView* render_view = GetCurrentRenderView();
    if (render_view == NULL)
      return;

    bool success = render_view->Send(new AtomViewHostMsg_Message(
        render_view->GetRoutingID(), channel, arguments));

    if (!success)
      args->ThrowError("Unable to send AtomViewHostMsg_Message");
  }

  base::string16 IPCSendSync(mate::Arguments* args,
                          const base::string16& channel,
                          const base::ListValue& arguments) {
    base::string16 json;

    content::RenderView* render_view = GetCurrentRenderView();
    if (render_view == NULL)
      return json;

    IPC::SyncMessage* message = new AtomViewHostMsg_Message_Sync(
        render_view->GetRoutingID(), channel, arguments, &json);
    bool success = render_view->Send(message);

    if (!success)
      args->ThrowError("Unable to send AtomViewHostMsg_Message_Sync");

    return json;
  }
}  // namespace

namespace atom {

JavascriptBindings::JavascriptBindings(extensions::ScriptContext* context)
    : extensions::ObjectBackedNativeHandler(context) {
  RouteFunction(
      "GetBinding",
      base::Bind(&JavascriptBindings::GetBinding, base::Unretained(this)));
}

JavascriptBindings::~JavascriptBindings() {
}

void JavascriptBindings::GetBinding(
      const v8::FunctionCallbackInfo<v8::Value>& args) {
  blink::WebLocalFrame* frame = context()->web_frame();
  CHECK(frame);

  v8::Isolate* isolate = args.GetIsolate();
  v8::HandleScope handle_scope(isolate);

  v8::Local<v8::Context> context = frame->mainWorldScriptContext();
  v8::Context::Scope context_scope(context);

  v8::Local<v8::String> atom_binding_string(
      v8::String::NewFromUtf8(isolate, "atom_binding"));
  v8::Local<v8::Object> global(context->Global());
  v8::Local<v8::Value> atom_binding(
      global->GetHiddenValue(atom_binding_string));
  if (atom_binding.IsEmpty()) {
    mate::Dictionary binding(isolate, v8::Object::New(isolate));

    mate::Dictionary v8_util(isolate, v8::Object::New(isolate));
    v8_util.SetMethod("setHiddenValue", &SetHiddenValue);
    v8_util.SetMethod("getHiddenValue", &GetHiddenValue);
    binding.Set("v8_util", v8_util.GetHandle());

    mate::Dictionary ipc(isolate, v8::Object::New(isolate));
    ipc.SetMethod("send", &IPCSend);
    ipc.SetMethod("sendSync", &IPCSendSync);
    binding.Set("ipc", ipc.GetHandle());

    mate::Dictionary web_frame(isolate, v8::Object::New(isolate));
    web_frame.Set("webFrame", atom::api::WebFrame::Create(isolate));
    binding.Set("web_frame", web_frame);
    atom_binding = binding.GetHandle();
    global->SetHiddenValue(atom_binding_string, atom_binding);
  }
  args.GetReturnValue().Set(atom_binding);
}

}  // namespace atom
