// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

// Multiply-included file, no traditional include guard.

#include "atom/common/draggable_region.h"
#include "base/strings/string16.h"
#include "base/values.h"
#include "content/public/common/common_param_traits.h"
#include "ipc/ipc_message_macros.h"
#include "ui/gfx/ipc/gfx_param_traits.h"

// The message starter should be declared in ipc/ipc_message_start.h. Since
// we don't want to patch Chromium, we just pretend to be Content Shell.

#define IPC_MESSAGE_START ShellMsgStart

IPC_STRUCT_TRAITS_BEGIN(atom::DraggableRegion)
  IPC_STRUCT_TRAITS_MEMBER(draggable)
  IPC_STRUCT_TRAITS_MEMBER(bounds)
IPC_STRUCT_TRAITS_END()

IPC_MESSAGE_ROUTED2(AtomViewHostMsg_Message,
                    base::string16 /* channel */,
                    base::ListValue /* arguments */)

IPC_SYNC_MESSAGE_ROUTED2_1(AtomViewHostMsg_Message_Sync,
                           base::string16 /* channel */,
                           base::ListValue /* arguments */,
                           base::string16 /* result (in JSON) */)

IPC_MESSAGE_ROUTED3(AtomViewMsg_Message,
                    bool /* send_to_all */,
                    base::string16 /* channel */,
                    base::ListValue /* arguments */)

// Sent by the renderer when the draggable regions are updated.
IPC_MESSAGE_ROUTED1(AtomViewHostMsg_UpdateDraggableRegions,
                    std::vector<atom::DraggableRegion> /* regions */)

// Update renderer process preferences.
IPC_MESSAGE_CONTROL1(AtomMsg_UpdatePreferences, base::ListValue)

// Update renderer content settings
IPC_MESSAGE_CONTROL1(AtomMsg_UpdateContentSettings, base::DictionaryValue)

