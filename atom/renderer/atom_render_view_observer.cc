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
#include "atom/common/options_switches.h"
#include "atom/renderer/atom_renderer_client.h"
#include "base/command_line.h"
#include "base/strings/string_number_conversions.h"
#include "content/public/renderer/render_view.h"
#include "ipc/ipc_message_macros.h"
#include "net/base/net_module.h"
#include "net/grit/net_resources.h"
#include "third_party/WebKit/public/web/WebDraggableRegion.h"
#include "third_party/WebKit/public/web/WebDocument.h"
#include "third_party/WebKit/public/web/WebFrame.h"
#include "third_party/WebKit/public/web/WebLocalFrame.h"
#include "third_party/WebKit/public/web/WebKit.h"
#include "third_party/WebKit/public/web/WebView.h"
#include "ui/base/resource/resource_bundle.h"
#include "native_mate/dictionary.h"

namespace atom {

namespace {

bool GetIPCObject(v8::Isolate* isolate,
                  v8::Local<v8::Context> context,
                  v8::Local<v8::Object>* ipc) {
  v8::Local<v8::String> key = mate::StringToV8(isolate, "ipc");
  v8::Local<v8::Value> value = context->Global()->GetHiddenValue(key);
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

// TODO(bridiver) create a separate file for script functions
std::string ExceptionToString(const v8::TryCatch& try_catch) {
  std::string str;
  v8::HandleScope handle_scope(v8::Isolate::GetCurrent());
  v8::String::Utf8Value exception(try_catch.Exception());
  v8::Local<v8::Message> message(try_catch.Message());
  if (message.IsEmpty()) {
    str.append(base::StringPrintf("%s\n", *exception));
  } else {
    v8::String::Utf8Value filename(message->GetScriptOrigin().ResourceName());
    int linenum = message->GetLineNumber();
    int colnum = message->GetStartColumn();
    str.append(base::StringPrintf(
        "%s:%i:%i %s\n", *filename, linenum, colnum, *exception));
    v8::String::Utf8Value sourceline(message->GetSourceLine());
    str.append(base::StringPrintf("%s\n", *sourceline));
  }
  return str;
}

}  // namespace

AtomRenderViewObserver::AtomRenderViewObserver(
    content::RenderView* render_view,
    AtomRendererClient* renderer_client)
    : content::RenderViewObserver(render_view),
      renderer_client_(renderer_client),
      document_created_(false) {
  // Initialise resource for directory listing.
  net::NetModule::SetResourceProvider(NetResourceProvider);
}

AtomRenderViewObserver::~AtomRenderViewObserver() {
}

void AtomRenderViewObserver::DidCreateDocumentElement(
    blink::WebLocalFrame* frame) {
  document_created_ = true;

  // Read --zoom-factor from command line.
  std::string zoom_factor_str = base::CommandLine::ForCurrentProcess()->
      GetSwitchValueASCII(switches::kZoomFactor);
  if (zoom_factor_str.empty())
    return;
  double zoom_factor;
  if (!base::StringToDouble(zoom_factor_str, &zoom_factor))
    return;
  double zoom_level = blink::WebView::zoomFactorToZoomLevel(zoom_factor);
  frame->view()->setZoomLevel(zoom_level);
}

void AtomRenderViewObserver::DraggableRegionsChanged(blink::WebFrame* frame) {
  blink::WebVector<blink::WebDraggableRegion> webregions =
      frame->document().draggableRegions();
  std::vector<DraggableRegion> regions;
  for (size_t i = 0; i < webregions.size(); ++i) {
    DraggableRegion region;
    region.bounds = webregions[i].bounds;
    region.draggable = webregions[i].draggable;
    regions.push_back(region);
  }
  Send(new AtomViewHostMsg_UpdateDraggableRegions(routing_id(), regions));
}

bool AtomRenderViewObserver::OnMessageReceived(const IPC::Message& message) {
  bool handled = true;
  IPC_BEGIN_MESSAGE_MAP(AtomRenderViewObserver, message)
    IPC_MESSAGE_HANDLER(AtomViewMsg_Message, OnBrowserMessage)
    IPC_MESSAGE_UNHANDLED(handled = false)
  IPC_END_MESSAGE_MAP()

  return handled;
}

void AtomRenderViewObserver::OnBrowserMessage(const base::string16& channel,
                                              const base::ListValue& args) {
  if (!document_created_)
    return;

  if (!render_view()->GetWebView())
    return;

  blink::WebFrame* frame = render_view()->GetWebView()->mainFrame();
  if (!frame || frame->isWebRemoteFrame())
    return;

  v8::Isolate* isolate = blink::mainThreadIsolate();
  v8::HandleScope handle_scope(isolate);

  v8::Local<v8::Context> context = frame->mainWorldScriptContext();
  v8::Context::Scope context_scope(context);

  v8::Local<v8::Object> ipc;
  if (GetIPCObject(isolate, context, &ipc)) {
    auto args_vector = ListValueToVector(isolate, args);
    // Insert the Event object, event.sender is ipc.
    mate::Dictionary event = mate::Dictionary::CreateEmpty(isolate);
    event.Set("sender", ipc);
    args_vector.insert(args_vector.begin(), event.GetHandle());
    if (renderer_client_->run_node()) {
      mate::EmitEvent(isolate, ipc, channel, args_vector);
    } else {
      // There might be a better way to do this,
      // I just copied the code from event_emitter_caller.h
      std::vector<v8::Local<v8::Value>> concatenated_args =
        { mate::StringToV8(isolate, channel) };
      concatenated_args.reserve(1 + args_vector.size());
      concatenated_args.insert(concatenated_args.end(),
                                args_vector.begin(), args_vector.end());

      v8::Handle<v8::Value> emit = ipc->Get(mate::StringToV8(isolate, "emit"));
      v8::Handle<v8::Function> fn = v8::Handle<v8::Function>::Cast(emit);

      v8::TryCatch try_catch;
      v8::Local<v8::Value> result = fn->Call(ipc, concatenated_args.size(),
                                                  &concatenated_args.front());
      if (result.IsEmpty()) {
        LOG(WARNING) << "Failed to execute ipc emit for " << channel;
        LOG(FATAL) << ExceptionToString(try_catch);
      }
    }
  }
}

}  // namespace atom
