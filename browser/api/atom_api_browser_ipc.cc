// Copyright (c) 2013 GitHub, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "browser/api/atom_api_browser_ipc.h"

#include "base/values.h"
#include "common/api/api_messages.h"
#include "common/v8_conversions.h"
#include "common/v8_value_converter_impl.h"
#include "content/public/browser/render_view_host.h"
#include "vendor/node/src/node.h"
#include "vendor/node/src/node_internals.h"

using content::RenderViewHost;
using content::V8ValueConverter;

namespace atom {

namespace api {

// static
v8::Handle<v8::Value> BrowserIPC::Send(const v8::Arguments &args) {
  v8::HandleScope scope;

  string16 channel;
  int process_id, routing_id;
  if (!FromV8Arguments(args, &channel, &process_id, &routing_id))
    return node::ThrowTypeError("Bad argument");

  RenderViewHost* render_view_host(RenderViewHost::FromID(
      process_id, routing_id));
  if (!render_view_host)
    return node::ThrowError("Invalid render view host");

  // Convert Arguments to Array, so we can use V8ValueConverter to convert it
  // to ListValue.
  v8::Local<v8::Array> v8_args = v8::Array::New(args.Length() - 3);
  for (int i = 0; i < args.Length() - 3; ++i)
    v8_args->Set(i, args[i + 3]);

  scoped_ptr<V8ValueConverter> converter(new V8ValueConverterImpl());
  scoped_ptr<base::Value> arguments(
      converter->FromV8Value(v8_args, v8::Context::GetCurrent()));

  DCHECK(arguments && arguments->IsType(base::Value::TYPE_LIST));

  render_view_host->Send(new AtomViewMsg_Message(
      routing_id,
      channel,
      *static_cast<base::ListValue*>(arguments.get())));

  return v8::Undefined();
}

// static
void BrowserIPC::Initialize(v8::Handle<v8::Object> target) {
  node::SetMethod(target, "send", Send);
}

}  // namespace api

}  // namespace atom

NODE_MODULE(atom_browser_ipc, atom::api::BrowserIPC::Initialize)
