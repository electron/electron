// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/common/javascript_bindings.h"

#include <vector>
#include "atom/browser/web_contents_preferences.h"
#include "atom/common/api/api_messages.h"
#include "atom/common/native_mate_converters/string16_converter.h"
#include "atom/common/native_mate_converters/value_converter.h"
#include "atom/renderer/api/atom_api_web_frame.h"
#include "atom/renderer/content_settings_manager.h"
#include "content/public/renderer/render_frame.h"
#include "content/public/renderer/render_view.h"
#include "extensions/renderer/script_context.h"
#include "native_mate/dictionary.h"
#include "third_party/WebKit/public/web/WebKit.h"
#include "third_party/WebKit/public/web/WebView.h"
#include "third_party/WebKit/public/web/WebLocalFrame.h"
#include "third_party/WebKit/public/web/WebDocument.h"

namespace {
  // TODO(bridiver) This is copied from atom_api_renderer_ipc.cc
  // and should be cleaned up
  v8::Local<v8::Value> GetHiddenValue(v8::Isolate* isolate,
                                      v8::Local<v8::Object> object,
                                      v8::Local<v8::String> key) {
    v8::Local<v8::Context> context = isolate->GetCurrentContext();
    v8::Local<v8::Private> privateKey = v8::Private::ForApi(isolate, key);
    v8::Local<v8::Value> value;
    v8::Maybe<bool> result = object->HasPrivate(context, privateKey);
    if (!(result.IsJust() && result.FromJust()))
      return v8::Local<v8::Value>();
    if (object->GetPrivate(context, privateKey).ToLocal(&value))
      return value;
    return v8::Local<v8::Value>();
  }

  void SetHiddenValue(v8::Isolate* isolate,
                      v8::Local<v8::Object> object,
                      v8::Local<v8::String> key,
                      v8::Local<v8::Value> value) {
    if (value.IsEmpty())
      return;
    v8::Local<v8::Context> context = isolate->GetCurrentContext();
    v8::Local<v8::Private> privateKey = v8::Private::ForApi(isolate, key);
    object->SetPrivate(context, privateKey, value);
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

  std::vector<v8::Local<v8::Value>> ListValueToVector(v8::Isolate* isolate,
                                                  const base::ListValue& list) {
    v8::Local<v8::Value> array = mate::ConvertToV8(isolate, list);
    std::vector<v8::Local<v8::Value>> result;
    mate::ConvertFromV8(isolate, array, &result);
    return result;
  }
}  // namespace

namespace atom {

JavascriptBindings::JavascriptBindings(content::RenderView* render_view,
                                       extensions::ScriptContext* context)
    : content::RenderViewObserver(render_view),
      extensions::ObjectBackedNativeHandler(context) {
  RouteFunction(
      "GetBinding",
      base::Bind(&JavascriptBindings::GetBinding, base::Unretained(this)));
}

JavascriptBindings::~JavascriptBindings() {
}

void JavascriptBindings::OnDestruct() {
  // do nothing
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
      GetHiddenValue(isolate, global, atom_binding_string));
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

    mate::Dictionary content_settings(isolate, v8::Object::New(isolate));
    content_settings.SetMethod("get",
        base::Bind(&ContentSettingsManager::GetSetting,
          base::Unretained(ContentSettingsManager::GetInstance())));
    content_settings.SetMethod("getContentTypes",
        base::Bind(&ContentSettingsManager::GetContentTypes,
          base::Unretained(ContentSettingsManager::GetInstance())));
    content_settings.SetMethod("getCurrent",
        base::Bind(&ContentSettingsManager::GetSetting,
          base::Unretained(ContentSettingsManager::GetInstance()),
          render_view()->GetWebView()->mainFrame()->document().url(),
          frame->document().url()));
    binding.Set("content_settings", content_settings);

    atom_binding = binding.GetHandle();
    SetHiddenValue(isolate, global, atom_binding_string, atom_binding);
  }
  args.GetReturnValue().Set(atom_binding);
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
