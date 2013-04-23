// Copyright (c) 2013 GitHub, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Multiply-included file, no traditional include guard.

#include <string>

#include "base/values.h"
#include "content/public/common/common_param_traits.h"
#include "ipc/ipc_message_macros.h"

// The message starter should be declared in ipc/ipc_message_start.h. Since
// we don't wan't to patch Chromium, we just pretend to be Content Shell.

#define IPC_MESSAGE_START ShellMsgStart

IPC_MESSAGE_ROUTED2(AtomViewHostMsg_Message,
                    std::string /* channel */,
                    ListValue /* arguments */)

IPC_SYNC_MESSAGE_ROUTED2_1(AtomViewHostMsg_Message_Sync,
                           std::string /* channel */,
                           ListValue /* arguments */,
                           DictionaryValue /* result */)

IPC_MESSAGE_ROUTED2(AtomViewMsg_Message,
                    std::string /* channel */,
                    ListValue /* arguments */)

IPC_SYNC_MESSAGE_ROUTED2_1(AtomViewMsg_Message_Sync,
                           std::string /* channel */,
                           ListValue /* arguments */,
                           DictionaryValue /* result */)
