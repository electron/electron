// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/renderer/api/atom_renderer_bindings.h"

#include <vector>

#include "atom/common/native_mate_converters/string16_converter.h"
#include "atom/common/native_mate_converters/v8_value_converter.h"
#include "base/memory/scoped_ptr.h"
#include "base/values.h"
#include "content/public/renderer/render_view.h"
#include "native_mate/converter.h"
#include "third_party/WebKit/public/web/WebFrame.h"
#include "third_party/WebKit/public/web/WebView.h"

#include "atom/common/node_includes.h"

namespace atom {

namespace {

v8::Handle<v8::Object> GetProcessObject(v8::Handle<v8::Context> context) {
  v8::Handle<v8::Object> process = context->Global()->Get(
      mate::StringToV8(context->GetIsolate(), "process"))->ToObject();
  DCHECK(!process.IsEmpty());

  return process;
}

}  // namespace

AtomRendererBindings::AtomRendererBindings() {
}

AtomRendererBindings::~AtomRendererBindings() {
}

void AtomRendererBindings::BindToFrame(blink::WebFrame* frame) {
  v8::Isolate* isolate = v8::Isolate::GetCurrent();
  v8::HandleScope handle_scope(isolate);

  v8::Handle<v8::Context> context = frame->mainWorldScriptContext();
  if (context.IsEmpty())
    return;

  v8::Context::Scope scope(context);
  AtomBindings::BindTo(isolate, GetProcessObject(context));
}

void AtomRendererBindings::OnBrowserMessage(content::RenderView* render_view,
                                            const base::string16& channel,
                                            const base::ListValue& args) {
  if (!render_view->GetWebView())
    return;

  v8::Isolate* isolate = v8::Isolate::GetCurrent();
  v8::HandleScope handle_scope(isolate);

  v8::Local<v8::Context> context =
      render_view->GetWebView()->mainFrame()->mainWorldScriptContext();
  if (context.IsEmpty())
    return;

  v8::Context::Scope context_scope(context);

  v8::Handle<v8::Object> process = GetProcessObject(context);
  scoped_ptr<V8ValueConverter> converter(new V8ValueConverter);

  std::vector<v8::Handle<v8::Value>> arguments;
  arguments.reserve(1 + args.GetSize());
  arguments.push_back(mate::ConvertToV8(isolate, channel));

  for (size_t i = 0; i < args.GetSize(); i++) {
    const base::Value* value;
    if (args.Get(i, &value))
      arguments.push_back(converter->ToV8Value(value, context));
  }

  node::MakeCallback(isolate, process, "emit", arguments.size(), &arguments[0]);
}

}  // namespace atom
