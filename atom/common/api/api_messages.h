// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

// Multiply-included file, no traditional include guard.

#include "base/strings/string16.h"
#include "base/values.h"
#include "content/public/common/common_param_traits.h"
#include "ipc/ipc_message_macros.h"
#include "ui/gfx/geometry/rect_f.h"
#include "ui/gfx/ipc/gfx_param_traits.h"

// The message starter should be declared in ipc/ipc_message_start.h. Since
// we don't want to patch Chromium, we just pretend to be Content Shell.

#define IPC_MESSAGE_START ElectronMsgStart

IPC_MESSAGE_ROUTED3(AtomAutofillFrameHostMsg_ShowPopup,
                    gfx::RectF /* bounds */,
                    std::vector<base::string16> /* values */,
                    std::vector<base::string16> /* labels */)

IPC_MESSAGE_ROUTED0(AtomAutofillFrameHostMsg_HidePopup)

IPC_MESSAGE_ROUTED1(AtomAutofillFrameMsg_AcceptSuggestion,
                    base::string16 /* suggestion */)

// Update renderer process preferences.
IPC_MESSAGE_CONTROL1(AtomMsg_UpdatePreferences, base::ListValue)
