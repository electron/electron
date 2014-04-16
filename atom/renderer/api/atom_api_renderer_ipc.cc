// Copyright (c) 2013 GitHub, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "atom/common/api/api_messages.h"
#include "atom/common/v8/v8_value_converter.h"
#include "content/public/renderer/render_view.h"
#include "native_mate/object_template_builder.h"
#include "third_party/WebKit/public/web/WebFrame.h"
#include "third_party/WebKit/public/web/WebView.h"

#include "atom/common/v8/node_common.h"

using content::RenderView;
using WebKit::WebFrame;
using WebKit::WebView;

namespace mate {

template<>
struct Converter<string16> {
  static v8::Handle<v8::Value> ToV8(v8::Isolate* isolate,
                                    const string16& val) {
    return v8::String::New(reinterpret_cast<const uint16_t*>(val.data()),
                           val.size());
  }
  static bool FromV8(v8::Isolate* isolate,
                     v8::Handle<v8::Value> val,
                     string16* out) {
    v8::String::Value s(val);
    *out = string16(reinterpret_cast<const char16*>(*s), s.length());
    return true;
  }
};

template<>
struct Converter<base::ListValue> {
  static bool FromV8(v8::Isolate* isolate,
                     v8::Handle<v8::Value> val,
                     base::ListValue* out) {
    scoped_ptr<atom::V8ValueConverter> converter(new atom::V8ValueConverter);
    scoped_ptr<base::Value> value(converter->FromV8Value(
        val, v8::Context::GetCurrent()));
    if (value->IsType(base::Value::TYPE_LIST)) {
      out->Swap(static_cast<ListValue*>(value.get()));
      return true;
    } else {
      return false;
    }
  }
};

}  // namespace mate

namespace {

RenderView* GetCurrentRenderView() {
  WebFrame* frame = WebFrame::frameForCurrentContext();
  if (!frame)
    return NULL;

  WebView* view = frame->view();
  if (!view)
    return NULL;  // can happen during closing.

  return RenderView::FromWebView(view);
}

void Send(const string16& channel, const base::ListValue& arguments) {
  RenderView* render_view = GetCurrentRenderView();
  if (render_view == NULL)
    return;

  bool success = render_view->Send(new AtomViewHostMsg_Message(
      render_view->GetRoutingID(), channel, arguments));

  if (!success)
    node::ThrowError("Unable to send AtomViewHostMsg_Message");
}

string16 SendSync(const string16& channel, const base::ListValue& arguments) {
  string16 json;

  RenderView* render_view = GetCurrentRenderView();
  if (render_view == NULL)
    return json;

  IPC::SyncMessage* message = new AtomViewHostMsg_Message_Sync(
      render_view->GetRoutingID(), channel, arguments, &json);
  // Enable the UI thread in browser to receive messages.
  message->EnableMessagePumping();
  bool success = render_view->Send(message);

  if (!success)
    node::ThrowError("Unable to send AtomViewHostMsg_Message_Sync");

  return json;
}

void Initialize(v8::Handle<v8::Object> exports) {
  mate::ObjectTemplateBuilder builder(v8::Isolate::GetCurrent());
  builder.SetMethod("send", &Send)
         .SetMethod("sendSync", &SendSync);
  exports->SetPrototype(builder.Build()->NewInstance());
}

}  // namespace

NODE_MODULE(atom_renderer_ipc, Initialize)
