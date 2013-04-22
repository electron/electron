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

IPC_SYNC_MESSAGE_CONTROL2_1(AtomViewHostMsg_Allocate_Object,
                            std::string /* module */,
                            DictionaryValue /* options */,
                            int /* object id */)

IPC_SYNC_MESSAGE_CONTROL1_0(AtomViewHostMsg_Deallocate_Object,
                            int /* object id */)

IPC_SYNC_MESSAGE_CONTROL3_1(AtomViewHostMsg_Call_Object_Method,
                            int /* object id */,
                            std::string /* method name */,
                            ListValue /* arguments */,
                            DictionaryValue /* return value */)

IPC_SYNC_MESSAGE_CONTROL2_1(AtomViewMsg_Callback,
                            int /* callback id */,
                            ListValue /* arguments */,
                            DictionaryValue /* return value */)
