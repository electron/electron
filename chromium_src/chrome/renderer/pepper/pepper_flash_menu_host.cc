// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/renderer/pepper/pepper_flash_menu_host.h"

#include "base/strings/utf_string_conversions.h"
#include "content/public/common/context_menu_params.h"
#include "content/public/renderer/render_frame.h"
#include "content/public/renderer/renderer_ppapi_host.h"
#include "ipc/ipc_message.h"
#include "ppapi/c/private/ppb_flash_menu.h"
#include "ppapi/host/dispatch_host_message.h"
#include "ppapi/host/ppapi_host.h"
#include "ppapi/proxy/ppapi_messages.h"
#include "ppapi/proxy/serialized_flash_menu.h"
#include "ui/gfx/geometry/point.h"

namespace {

// Maximum depth of submenus allowed (e.g., 1 indicates that submenus are
// allowed, but not sub-submenus).
const size_t kMaxMenuDepth = 2;

// Maximum number of entries in any single menu (including separators).
const size_t kMaxMenuEntries = 50;

// Maximum total number of entries in the |menu_id_map| (see below).
// (Limit to 500 real entries; reserve the 0 action as an invalid entry.)
const size_t kMaxMenuIdMapEntries = 501;

// Converts menu data from one form to another.
//  - |depth| is the current nested depth (call it starting with 0).
//  - |menu_id_map| is such that |menu_id_map[output_item.action] ==
//    input_item.id| (where |action| is what a |MenuItem| has, |id| is what a
//    |PP_Flash_MenuItem| has).
bool ConvertMenuData(const PP_Flash_Menu* in_menu,
                     size_t depth,
                     std::vector<content::MenuItem>* out_menu,
                     std::vector<int32_t>* menu_id_map) {
  if (depth > kMaxMenuDepth || !in_menu)
    return false;

  // Clear the output, just in case.
  out_menu->clear();

  if (!in_menu->count)
    return true;  // Nothing else to do.

  if (!in_menu->items || in_menu->count > kMaxMenuEntries)
    return false;
  for (uint32_t i = 0; i < in_menu->count; i++) {
    content::MenuItem item;

    PP_Flash_MenuItem_Type type = in_menu->items[i].type;
    switch (type) {
      case PP_FLASH_MENUITEM_TYPE_NORMAL:
        item.type = content::MenuItem::OPTION;
        break;
      case PP_FLASH_MENUITEM_TYPE_CHECKBOX:
        item.type = content::MenuItem::CHECKABLE_OPTION;
        break;
      case PP_FLASH_MENUITEM_TYPE_SEPARATOR:
        item.type = content::MenuItem::SEPARATOR;
        break;
      case PP_FLASH_MENUITEM_TYPE_SUBMENU:
        item.type = content::MenuItem::SUBMENU;
        break;
      default:
        return false;
    }
    if (in_menu->items[i].name)
      item.label = base::UTF8ToUTF16(in_menu->items[i].name);
    if (menu_id_map->size() >= kMaxMenuIdMapEntries)
      return false;
    item.action = static_cast<unsigned>(menu_id_map->size());
    // This sets |(*menu_id_map)[item.action] = in_menu->items[i].id|.
    menu_id_map->push_back(in_menu->items[i].id);
    item.enabled = PP_ToBool(in_menu->items[i].enabled);
    item.checked = PP_ToBool(in_menu->items[i].checked);
    if (type == PP_FLASH_MENUITEM_TYPE_SUBMENU) {
      if (!ConvertMenuData(
              in_menu->items[i].submenu, depth + 1, &item.submenu, menu_id_map))
        return false;
    }

    out_menu->push_back(item);
  }

  return true;
}

}  // namespace

PepperFlashMenuHost::PepperFlashMenuHost(
    content::RendererPpapiHost* host,
    PP_Instance instance,
    PP_Resource resource,
    const ppapi::proxy::SerializedFlashMenu& serial_menu)
    : ppapi::host::ResourceHost(host->GetPpapiHost(), instance, resource),
      renderer_ppapi_host_(host),
      showing_context_menu_(false),
      context_menu_request_id_(0),
      has_saved_context_menu_action_(false),
      saved_context_menu_action_(0) {
  menu_id_map_.push_back(0);  // Reserve |menu_id_map_[0]|.
  if (!ConvertMenuData(serial_menu.pp_menu(), 0, &menu_data_, &menu_id_map_)) {
    menu_data_.clear();
    menu_id_map_.clear();
  }
}

PepperFlashMenuHost::~PepperFlashMenuHost() {
  if (showing_context_menu_) {
    content::RenderFrame* render_frame =
        renderer_ppapi_host_->GetRenderFrameForInstance(pp_instance());
    if (render_frame)
      render_frame->CancelContextMenu(context_menu_request_id_);
  }
}

int32_t PepperFlashMenuHost::OnResourceMessageReceived(
    const IPC::Message& msg,
    ppapi::host::HostMessageContext* context) {
  PPAPI_BEGIN_MESSAGE_MAP(PepperFlashMenuHost, msg)
    PPAPI_DISPATCH_HOST_RESOURCE_CALL(PpapiHostMsg_FlashMenu_Show,
                                      OnHostMsgShow)
  PPAPI_END_MESSAGE_MAP()
  return PP_ERROR_FAILED;
}

int32_t PepperFlashMenuHost::OnHostMsgShow(
    ppapi::host::HostMessageContext* context,
    const PP_Point& location) {
  // Note that all early returns must do a SendMenuReply. The sync result for
  // this message isn't used, so to forward the error to the plugin, we need to
  // additionally call SendMenuReply explicitly.
  if (menu_data_.empty()) {
    SendMenuReply(PP_ERROR_FAILED, -1);
    return PP_ERROR_FAILED;
  }
  if (showing_context_menu_) {
    SendMenuReply(PP_ERROR_INPROGRESS, -1);
    return PP_ERROR_INPROGRESS;
  }

  content::RenderFrame* render_frame =
      renderer_ppapi_host_->GetRenderFrameForInstance(pp_instance());

  content::ContextMenuParams params;
  params.x = location.x;
  params.y = location.y;
  params.custom_context.is_pepper_menu = true;
  params.custom_context.render_widget_id =
      renderer_ppapi_host_->GetRoutingIDForWidget(pp_instance());
  params.custom_items = menu_data_;

  // Transform the position to be in render frame's coordinates.
  gfx::Point render_frame_pt = renderer_ppapi_host_->PluginPointToRenderFrame(
      pp_instance(), gfx::Point(location.x, location.y));
  params.x = render_frame_pt.x();
  params.y = render_frame_pt.y();

  showing_context_menu_ = true;
  context_menu_request_id_ = render_frame->ShowContextMenu(this, params);

  // Note: the show message is sync so this OK is for the sync reply which we
  // don't actually use (see the comment in the resource file for this). The
  // async message containing the context menu action will be sent in the
  // future.
  return PP_OK;
}

void PepperFlashMenuHost::OnMenuAction(int request_id, unsigned action) {
  // Just save the action.
  DCHECK(!has_saved_context_menu_action_);
  has_saved_context_menu_action_ = true;
  saved_context_menu_action_ = action;
}

void PepperFlashMenuHost::OnMenuClosed(int request_id) {
  if (has_saved_context_menu_action_ &&
      saved_context_menu_action_ < menu_id_map_.size()) {
    SendMenuReply(PP_OK, menu_id_map_[saved_context_menu_action_]);
    has_saved_context_menu_action_ = false;
    saved_context_menu_action_ = 0;
  } else {
    SendMenuReply(PP_ERROR_USERCANCEL, -1);
  }

  showing_context_menu_ = false;
  context_menu_request_id_ = 0;
}

void PepperFlashMenuHost::SendMenuReply(int32_t result, int action) {
  ppapi::host::ReplyMessageContext reply_context(
      ppapi::proxy::ResourceMessageReplyParams(pp_resource(), 0),
      NULL,
      MSG_ROUTING_NONE);
  reply_context.params.set_result(result);
  host()->SendReply(reply_context, PpapiPluginMsg_FlashMenu_ShowReply(action));
}
