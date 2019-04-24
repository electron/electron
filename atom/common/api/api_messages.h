// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

// Multiply-included file, no traditional include guard.

#include "atom/common/draggable_region.h"
#include "base/strings/string16.h"
#include "base/values.h"
#include "content/public/common/common_param_traits.h"
#include "content/public/common/referrer.h"
#include "ipc/ipc_message_macros.h"
#include "ipc/ipc_platform_file.h"
#include "ui/gfx/geometry/rect_f.h"
#include "ui/gfx/ipc/gfx_param_traits.h"
#include "url/gurl.h"

// The message starter should be declared in ipc/ipc_message_start.h. Since
// we don't want to patch Chromium, we just pretend to be Content Shell.

#define IPC_MESSAGE_START ElectronMsgStart

IPC_STRUCT_TRAITS_BEGIN(atom::DraggableRegion)
  IPC_STRUCT_TRAITS_MEMBER(draggable)
  IPC_STRUCT_TRAITS_MEMBER(bounds)
IPC_STRUCT_TRAITS_END()

IPC_MESSAGE_ROUTED3(AtomAutofillFrameHostMsg_ShowPopup,
                    gfx::RectF /* bounds */,
                    std::vector<base::string16> /* values */,
                    std::vector<base::string16> /* labels */)

IPC_MESSAGE_ROUTED0(AtomAutofillFrameHostMsg_HidePopup)

IPC_MESSAGE_ROUTED1(AtomAutofillFrameMsg_AcceptSuggestion,
                    base::string16 /* suggestion */)

// Sent by the renderer when the draggable regions are updated.
IPC_MESSAGE_ROUTED1(AtomFrameHostMsg_UpdateDraggableRegions,
                    std::vector<atom::DraggableRegion> /* regions */)

// Update renderer process preferences.
IPC_MESSAGE_CONTROL1(AtomMsg_UpdatePreferences, base::ListValue)

// Sent by renderer to set the temporary zoom level.
IPC_SYNC_MESSAGE_ROUTED1_1(AtomFrameHostMsg_SetTemporaryZoomLevel,
                           double /* zoom level */,
                           double /* result */)

// Sent by renderer to get the zoom level.
IPC_SYNC_MESSAGE_ROUTED0_1(AtomFrameHostMsg_GetZoomLevel, double /* result */)
