// Copyright (c) 2013 GitHub, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "browser/api/atom_api_browser_ipc.h"

#include "common/api/api_messages.h"
#include "common/v8_conversions.h"
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
  scoped_ptr<base::Value> arguments;
  if (!FromV8Arguments(args, &channel, &process_id, &routing_id, &arguments))
    return node::ThrowTypeError("Bad argument");

  DCHECK(arguments && arguments->IsType(base::Value::TYPE_LIST));

  RenderViewHost* render_view_host(RenderViewHost::FromID(
      process_id, routing_id));
  if (!render_view_host)
    return node::ThrowError("Invalid render view host");

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
