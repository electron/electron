// Copyright (c) 2016 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/atom_permission_manager.h"

#include "content/public/browser/child_process_security_policy.h"
#include "content/public/browser/permission_type.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/browser/web_contents.h"

namespace atom {

AtomPermissionManager::AtomPermissionManager()
    : request_id_(0) {
}

AtomPermissionManager::~AtomPermissionManager() {
}

void AtomPermissionManager::SetPermissionRequestHandler(
    const RequestHandler& handler) {
  if (handler.is_null() && !pending_requests_.empty()) {
    for (const auto& request : pending_requests_)
      request.second.Run(content::PERMISSION_STATUS_DENIED);
    pending_requests_.clear();
  }
  request_handler_ = handler;
}

int AtomPermissionManager::RequestPermission(
    content::PermissionType permission,
    content::RenderFrameHost* render_frame_host,
    const GURL& requesting_origin,
    bool user_gesture,
    const ResponseCallback& response_callback) {
  if (permission == content::PermissionType::MIDI_SYSEX) {
    content::ChildProcessSecurityPolicy::GetInstance()->
        GrantSendMidiSysExMessage(render_frame_host->GetProcess()->GetID());
  }

  if (!request_handler_.is_null()) {
    auto web_contents =
        content::WebContents::FromRenderFrameHost(render_frame_host);
    ++request_id_;
    auto callback = base::Bind(&AtomPermissionManager::OnPermissionResponse,
                               base::Unretained(this),
                               request_id_,
                               requesting_origin,
                               response_callback);
    pending_requests_[request_id_] = callback;
    request_handler_.Run(web_contents, permission, callback);
    return request_id_;
  }

  response_callback.Run(content::PERMISSION_STATUS_GRANTED);
  return kNoPendingOperation;
}

void AtomPermissionManager::OnPermissionResponse(
    int request_id,
    const GURL& origin,
    const ResponseCallback& callback,
    content::PermissionStatus status) {
  callback.Run(status);
  pending_requests_.erase(request_id);
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
