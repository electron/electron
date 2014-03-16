// Copyright (c) 2013 GitHub, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Multiply-included file, no traditional include guard.

#include "base/strings/string16.h"
#include "base/values.h"
#include "atom/common/draggable_region.h"
#include "content/public/common/common_param_traits.h"
#include "ipc/ipc_message_macros.h"

// The message starter should be declared in ipc/ipc_message_start.h. Since
// we don't wan't to patch Chromium, we just pretend to be Content Shell.

#define IPC_MESSAGE_START ShellMsgStart

IPC_STRUCT_TRAITS_BEGIN(atom::DraggableRegion)
  IPC_STRUCT_TRAITS_MEMBER(draggable)
  IPC_STRUCT_TRAITS_MEMBER(bounds)
IPC_STRUCT_TRAITS_END()

IPC_MESSAGE_ROUTED2(AtomViewHostMsg_Message,
                    string16 /* channel */,
                    ListValue /* arguments */)

IPC_SYNC_MESSAGE_ROUTED2_1(AtomViewHostMsg_Message_Sync,
                           string16 /* channel */,
                           ListValue /* arguments */,
                           string16 /* result (in JSON) */)

IPC_MESSAGE_ROUTED2(AtomViewMsg_Message,
                    string16 /* channel */,
                    ListValue /* arguments */)

// Sent by the renderer when the draggable regions are updated.
IPC_MESSAGE_ROUTED1(AtomViewHostMsg_UpdateDraggableRegions,
                    std::vector<atom::DraggableRegion> /* regions */)
