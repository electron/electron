// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/renderer/extensions/tabs_custom_bindings.h"

#include <stdint.h>

#include <string>

#include "base/bind.h"
#include "content/public/renderer/render_frame.h"
#include "extensions/common/extension_messages.h"
#include "extensions/renderer/script_context.h"
#include "v8/include/v8.h"

namespace extensions {

TabsCustomBindings::TabsCustomBindings(ScriptContext* context)
    : ObjectBackedNativeHandler(context) {
  RouteFunction("OpenChannelToTab",
      base::Bind(&TabsCustomBindings::OpenChannelToTab,
                 base::Unretained(this)));
}

void TabsCustomBindings::OpenChannelToTab(
    const v8::FunctionCallbackInfo<v8::Value>& args) {
  content::RenderFrame* render_frame = context()->GetRenderFrame();
  if (!render_frame)
    return;

  // tabs_custom_bindings.js unwraps arguments to tabs.connect/sendMessage and
  // passes them to OpenChannelToTab, in the following order:
  // - |tab_id| - Positive number that specifies the destination of the channel.
  // - |frame_id| - Target frame(s) in the tab where onConnect is dispatched:
  //   -1 for all frames, 0 for the main frame, >0 for a child frame.
  // - |extension_id| - Extension ID of sender and destination.
  // - |channel_name| - A user-defined channel name.
  CHECK(args.Length() >= 4 &&
        args[0]->IsInt32() &&
        args[1]->IsInt32() &&
        args[2]->IsString() &&
        args[3]->IsString());

  ExtensionMsg_TabTargetConnectionInfo info;
  info.tab_id = args[0]->Int32Value();
  info.frame_id = args[1]->Int32Value();
  std::string extension_id = *v8::String::Utf8Value(args[2]);
  std::string channel_name = *v8::String::Utf8Value(args[3]);
  int port_id = -1;
  render_frame->Send(new ExtensionHostMsg_OpenChannelToTab(
      render_frame->GetRoutingID(), info, extension_id, channel_name,
      &port_id));
  args.GetReturnValue().Set(static_cast<int32_t>(port_id));
}

}  // namespace extensions
