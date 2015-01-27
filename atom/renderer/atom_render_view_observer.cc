// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/renderer/atom_render_view_observer.h"

#include <string>
#include <vector>

#include "atom/common/api/api_messages.h"
#include "atom/common/native_mate_converters/string16_converter.h"
#include "atom/common/native_mate_converters/value_converter.h"
#include "atom/common/options_switches.h"
#include "atom/renderer/atom_renderer_client.h"
#include "base/command_line.h"
#include "base/strings/string_number_conversions.h"
#include "content/public/renderer/render_view.h"
#include "ipc/ipc_message_macros.h"
#include "third_party/WebKit/public/web/WebDraggableRegion.h"
#include "third_party/WebKit/public/web/WebDocument.h"
#include "third_party/WebKit/public/web/WebFrame.h"
#include "third_party/WebKit/public/web/WebLocalFrame.h"
#include "third_party/WebKit/public/web/WebKit.h"
#include "third_party/WebKit/public/web/WebView.h"

#include "atom/common/node_includes.h"

namespace atom {

namespace {

bool GetIPCObject(v8::Isolate* isolate,
                  v8::Handle<v8::Context> context,
                  v8::Handle<v8::Object>* ipc) {
  v8::Handle<v8::String> key = mate::StringToV8(isolate, "ipc");
  v8::Handle<v8::Value> value = context->Global()->GetHiddenValue(key);
  if (value.IsEmpty() || !value->IsObject())
    return false;
  *ipc = value->ToObject();
  return true;
}

std::vector<v8::Handle<v8::Value>> ListValueToVector(
    v8::Isolate* isolate,
    const base::ListValue& list) {
  v8::Handle<v8::Value> array = mate::ConvertToV8(isolate, list);
  std::vector<v8::Handle<v8::Value>> result;
  mate::ConvertFromV8(isolate, array, &result);
  return result;
}

}  // namespace

AtomRenderViewObserver::AtomRenderViewObserver(
    content::RenderView* render_view,
    AtomRendererClient* renderer_client)
    : content::RenderViewObserver(render_view),
      renderer_client_(renderer_client),
      document_created_(false) {
}

AtomRenderViewObserver::~AtomRenderViewObserver() {
}

void AtomRenderViewObserver::DidCreateDocumentElement(
    blink::WebLocalFrame* frame) {
  document_created_ = true;

  // Read --zoom-factor from command line.
  std::string zoom_factor_str = CommandLine::ForCurrentProcess()->
      GetSwitchValueASCII(switches::kZoomFactor);;
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

  std::vector<v8::Handle<v8::Value>> arguments = ListValueToVector(
      isolate, args);
  arguments.insert(arguments.begin(), mate::ConvertToV8(isolate, channel));

  v8::Handle<v8::Object> ipc;
  if (GetIPCObject(isolate, context, &ipc))
    node::MakeCallback(isolate, ipc, "emit", arguments.size(), &arguments[0]);
}

}  // namespace atom
