// Copyright (c) 2016 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/atom_permission_manager.h"

#include "content/public/browser/child_process_security_policy.h"
#include "content/public/browser/permission_type.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/render_process_host.h"

namespace atom {

AtomPermissionManager::AtomPermissionManager()
    : request_id_(0) {
}

AtomPermissionManager::~AtomPermissionManager() {
}

void AtomPermissionManager::SetPermissionRequestHandler(
    int id,
    const RequestHandler& handler) {
  if (handler.is_null()) {
    request_handler_map_.erase(id);
    return;
  }
  request_handler_map_[id] = handler;
}

void AtomPermissionManager::RequestPermission(
    content::PermissionType permission,
    content::RenderFrameHost* render_frame_host,
    const GURL& origin,
    const base::Callback<void(bool)>& callback) {
  bool user_gesture = false;
  RequestPermission(permission, render_frame_host, origin, user_gesture,
                    base::Bind(&AtomPermissionManager::OnPermissionResponse,
                               base::Unretained(this), callback));
}

void AtomPermissionManager::OnPermissionResponse(
    const base::Callback<void(bool)>& callback,
    content::PermissionStatus status) {
  if (status == content::PERMISSION_STATUS_GRANTED)
    callback.Run(true);
  else
    callback.Run(false);
}

int AtomPermissionManager::RequestPermission(
    content::PermissionType permission,
    content::RenderFrameHost* render_frame_host,
    const GURL& requesting_origin,
    bool user_gesture,
    const ResponseCallback& callback) {
  int process_id = render_frame_host->GetProcess()->GetID();
  auto request_handler = request_handler_map_.find(process_id);

  if (permission == content::PermissionType::MIDI_SYSEX) {
    content::ChildProcessSecurityPolicy::GetInstance()->
        GrantSendMidiSysExMessage(process_id);
  }

  if (request_handler != request_handler_map_.end()) {
    pending_requests_[++request_id_] = callback;
    request_handler->second.Run(permission, callback);
    return request_id_;
  }

  callback.Run(content::PERMISSION_STATUS_GRANTED);
  return kNoPendingOperation;
}

void AtomPermissionManager::CancelPermissionRequest(int request_id) {
  auto request = pending_requests_.find(request_id);
  if (request != pending_requests_.end()) {
    request->second.Run(content::PERMISSION_STATUS_DENIED);
    pending_requests_.erase(request);
  }
}

void AtomPermissionManager::ResetPermission(
    content::PermissionType permission,
    const GURL& requesting_origin,
    const GURL& embedding_origin) {
}

content::PermissionStatus AtomPermissionManager::GetPermissionStatus(
    content::PermissionType permission,
    const GURL& requesting_origin,
    const GURL& embedding_origin) {
  return content::PERMISSION_STATUS_GRANTED;
}

void AtomPermissionManager::RegisterPermissionUsage(
    content::PermissionType permission,
    const GURL& requesting_origin,
    const GURL& embedding_origin) {
}

int AtomPermissionManager::SubscribePermissionStatusChange(
    content::PermissionType permission,
    const GURL& requesting_origin,
    const GURL& embedding_origin,
    const ResponseCallback& callback) {
  return -1;
}

void AtomPermissionManager::UnsubscribePermissionStatusChange(
    int subscription_id) {
}

}  // namespace atom
