// Copyright (c) 2013 GitHub, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "atom/browser/api/atom_api_browser_ipc.h"

#include "atom/common/api/api_messages.h"
#include "atom/common/node_includes.h"
#include "atom/common/v8/native_type_conversions.h"
#include "content/public/browser/render_view_host.h"

using content::RenderViewHost;

namespace atom {

namespace api {

// static
void BrowserIPC::Send(const v8::FunctionCallbackInfo<v8::Value>& args) {
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

  args.GetReturnValue().Set(render_view_host->Send(new AtomViewMsg_Message(
      routing_id,
      channel,
      *static_cast<base::ListValue*>(arguments.get()))));
}

// static
void BrowserIPC::Initialize(v8::Handle<v8::Object> target) {
  NODE_SET_METHOD(target, "send", Send);
}

}  // namespace api

}  // namespace atom

NODE_MODULE(atom_browser_ipc, atom::api::BrowserIPC::Initialize)
