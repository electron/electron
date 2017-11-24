// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/renderer/atom_render_view_observer.h"

#include <string>
#include <vector>

// Put this before event_emitter_caller.h to have string16 support.
#include "atom/common/native_mate_converters/string16_converter.h"

#include "atom/common/api/api_messages.h"
#include "atom/common/api/event_emitter_caller.h"
#include "atom/common/native_mate_converters/value_converter.h"
#include "atom/common/node_includes.h"
#include "atom/renderer/atom_renderer_client.h"
#include "base/command_line.h"
#include "base/strings/string_number_conversions.h"
#include "base/trace_event/trace_event.h"
#include "content/public/renderer/render_view.h"
#include "ipc/ipc_message_macros.h"
#include "native_mate/dictionary.h"
#include "net/base/net_module.h"
#include "net/grit/net_resources.h"
#include "third_party/WebKit/public/web/WebDocument.h"
#include "third_party/WebKit/public/web/WebElement.h"
#include "third_party/WebKit/public/web/WebFrame.h"
#include "third_party/WebKit/public/web/WebKit.h"
#include "third_party/WebKit/public/web/WebLocalFrame.h"
#include "third_party/WebKit/public/web/WebView.h"
#include "ui/base/resource/resource_bundle.h"

namespace atom {

namespace {

bool GetIPCObject(v8::Isolate* isolate,
                  v8::Local<v8::Context> context,
                  v8::Local<v8::Object>* ipc) {
  v8::Local<v8::String> key = mate::StringToV8(isolate, "ipc");
  v8::Local<v8::Private> privateKey = v8::Private::ForApi(isolate, key);
  v8::Local<v8::Object> global_object = context->Global();
  v8::Local<v8::Value> value;
  if (!global_object->GetPrivate(context, privateKey).ToLocal(&value))
    return false;
  if (value.IsEmpty() || !value->IsObject())
    return false;
  *ipc = value->ToObject();
  return true;
}

std::vector<v8::Local<v8::Value>> ListValueToVector(
    v8::Isolate* isolate,
    const base::ListValue& list) {
  v8::Local<v8::Value> array = mate::ConvertToV8(isolate, list);
  std::vector<v8::Local<v8::Value>> result;
  mate::ConvertFromV8(isolate, array, &result);
  return result;
}

base::StringPiece NetResourceProvider(int key) {
  if (key == IDR_DIR_HEADER_HTML) {
    base::StringPiece html_data =
        ui::ResourceBundle::GetSharedInstance().GetRawDataResource(
            IDR_DIR_HEADER_HTML);
    return html_data;
  }
  return base::StringPiece();
}

}  // namespace

AtomRenderViewObserver::AtomRenderViewObserver(
    content::RenderView* render_view,
    AtomRendererClient* renderer_client)
    : content::RenderViewObserver(render_view),
      renderer_client_(renderer_client) {
  // Initialise resource for directory listing.
  net::NetModule::SetResourceProvider(NetResourceProvider);
}

AtomRenderViewObserver::~AtomRenderViewObserver() {
}

void AtomRenderViewObserver::EmitIPCEvent(blink::WebLocalFrame* frame,
                                          const base::string16& channel,
                                          const base::ListValue& args) {
  if (!frame)
    return;

  v8::Isolate* isolate = blink::MainThreadIsolate();
  v8::HandleScope handle_scope(isolate);

  v8::Local<v8::Context> context = renderer_client_->GetContext(frame, isolate);
  v8::Context::Scope context_scope(context);

  // Only emit IPC event for context with node integration.
  node::Environment* env = node::Environment::GetCurrent(context);
  if (!env)
    return;

  v8::Local<v8::Object> ipc;
  if (GetIPCObject(isolate, context, &ipc)) {
    TRACE_EVENT0("devtools.timeline", "FunctionCall");
    auto args_vector = ListValueToVector(isolate, args);
    // Insert the Event object, event.sender is ipc.
    mate::Dictionary event = mate::Dictionary::CreateEmpty(isolate);
    event.Set("sender", ipc);
    args_vector.insert(args_vector.begin(), event.GetHandle());
    mate::EmitEvent(isolate, ipc, channel, args_vector);
  }
}

bool AtomRenderViewObserver::OnMessageReceived(const IPC::Message& message) {
  bool handled = true;
  IPC_BEGIN_MESSAGE_MAP(AtomRenderViewObserver, message)
    IPC_MESSAGE_HANDLER(AtomViewMsg_Message, OnBrowserMessage)
    IPC_MESSAGE_HANDLER(AtomViewMsg_Offscreen, OnOffscreen)
    IPC_MESSAGE_UNHANDLED(handled = false)
  IPC_END_MESSAGE_MAP()

  return handled;
}

void AtomRenderViewObserver::OnDestruct() {
  delete this;
}

void AtomRenderViewObserver::OnBrowserMessage(bool send_to_all,
                                              const base::string16& channel,
                                              const base::ListValue& args) {
  if (!render_view()->GetWebView())
    return;

  blink::WebFrame* frame = render_view()->GetWebView()->MainFrame();
  if (!frame || !frame->IsWebLocalFrame())
    return;

  // Don't handle browser messages before document element is created.
  // When we receive a message from the browser, we try to transfer it
  // to a web page, and when we do that Blink creates an empty
  // document element if it hasn't been created yet, and it makes our init
  // script to run while `window.location` is still "about:blank".
  blink::WebDocument document = frame->ToWebLocalFrame()->GetDocument();
  blink::WebElement html_element = document.DocumentElement();
  if (html_element.IsNull()) {
    return;
  }

  EmitIPCEvent(frame->ToWebLocalFrame(), channel, args);

  // Also send the message to all sub-frames.
  if (send_to_all) {
    for (blink::WebFrame* child = frame->FirstChild(); child;
         child = child->NextSibling())
      if (child->IsWebLocalFrame()) {
        EmitIPCEvent(child->ToWebLocalFrame(), channel, args);
      }
  }
}

void AtomRenderViewObserver::OnOffscreen() {
  blink::WebView::SetUseExternalPopupMenus(false);
}

}  // namespace atom
