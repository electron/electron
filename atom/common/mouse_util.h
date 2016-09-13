// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_COMMON_MOUSE_UTIL_H_
#define ATOM_COMMON_MOUSE_UTIL_H_

#include <string>
#include "content/common/cursors/webcursor.h"
#include "ipc/ipc_message_macros.h"

// IPC macros similar to the already existing ones in the chromium source.
// We need these to listen to the cursor change IPC message while still
// letting chromium handle the actual cursor change by setting handled = false.
#define IPC_MESSAGE_HANDLER_CODE(msg_class, member_func, code)                 \
  IPC_MESSAGE_FORWARD_CODE(msg_class, this,                                    \
    _IpcMessageHandlerClass::member_func, code)

#define IPC_MESSAGE_FORWARD_CODE(msg_class, obj, member_func, code)            \
    case msg_class::ID: {                                                      \
        TRACK_RUN_IN_THIS_SCOPED_REGION(member_func);                          \
        if (!msg_class::Dispatch(&ipc_message__, obj, this, param__,           \
                                 &member_func))                                \
          ipc_message__.set_dispatch_error();                                  \
        code;                                                                  \
      }                                                                        \
      break;

namespace atom {

// Returns the cursor's type as a string.
std::string CursorTypeToString(const content::WebCursor::CursorInfo& info);

}  // namespace atom

#endif  // ATOM_COMMON_MOUSE_UTIL_H_
