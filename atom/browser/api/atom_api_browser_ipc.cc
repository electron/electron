// Copyright (c) 2013 GitHub, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "atom/common/api/api_messages.h"
#include "atom/common/native_mate_converters/string16_converter.h"
#include "atom/common/native_mate_converters/value_converter.h"
#include "content/public/browser/render_view_host.h"
#include "native_mate/dictionary.h"

#include "atom/common/node_includes.h"

using content::RenderViewHost;

namespace {

bool Send(const string16& channel, int process_id, int routing_id,
          const base::ListValue& arguments) {
  RenderViewHost* render_view_host(RenderViewHost::FromID(
      process_id, routing_id));
  if (!render_view_host) {
    node::ThrowError("Invalid render view host");
    return false;
  }

  return render_view_host->Send(new AtomViewMsg_Message(routing_id, channel,
                                                        arguments));
}

void Initialize(v8::Handle<v8::Object> exports) {
  mate::Dictionary dict(v8::Isolate::GetCurrent(), exports);
  dict.SetMethod("send", &Send);
}

}  // namespace

NODE_MODULE(atom_browser_ipc, Initialize)
