// Copyright (c) 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE-CHROMIUM file.

#include "browser/permission_manager.h"

#include "base/callback.h"
#include "content/public/browser/child_process_security_policy.h"
#include "content/public/browser/permission_type.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/render_process_host.h"

namespace brightray {

PermissionManager::PermissionManager() {
}

PermissionManager::~PermissionManager() {
}

int PermissionManager::RequestPermission(
    content::PermissionType permission,
    content::RenderFrameHost* render_frame_host,
    const GURL& requesting_origin,
    const base::Callback<void(content::PermissionStatus)>& callback) {
  if (permission == content::PermissionType::MIDI_SYSEX) {
    content::ChildProcessSecurityPolicy::GetInstance()->
        GrantSendMidiSysExMessage(render_frame_host->GetProcess()->GetID());
  }
  callback.Run(content::PermissionStatus::GRANTED);
  return kNoPendingOperation;
}

int PermissionManager::RequestPermissions(
    const std::vector<content::PermissionType>& permissions,
    content::RenderFrameHost* render_frame_host,
    const GURL& requesting_origin,
    const base::Callback<void(
        const std::vector<content::PermissionStatus>&)>& callback) {
  std::vector<content::PermissionStatus> permissionStatuses;

  for (auto permission : permissions) {
    if (permission == content::PermissionType::MIDI_SYSEX) {
      content::ChildProcessSecurityPolicy::GetInstance()->
          GrantSendMidiSysExMessage(render_frame_host->GetProcess()->GetID());
    }

    permissionStatuses.push_back(content::PermissionStatus::GRANTED);
  }

  callback.Run(permissionStatuses);
  return kNoPendingOperation;
}

void PermissionManager::CancelPermissionRequest(int request_id) {
}

void PermissionManager::ResetPermission(
    content::PermissionType permission,
    const GURL& requesting_origin,
    const GURL& embedding_origin) {
}

content::PermissionStatus PermissionManager::GetPermissionStatus(
    content::PermissionType permission,
    const GURL& requesting_origin,
    const GURL& embedding_origin) {
  return content::PermissionStatus::GRANTED;
}

void PermissionManager::RegisterPermissionUsage(
    content::PermissionType permission,
    const GURL& requesting_origin,
    const GURL& embedding_origin) {
}

int PermissionManager::SubscribePermissionStatusChange(
    content::PermissionType permission,
    const GURL& requesting_origin,
    const GURL& embedding_origin,
    const base::Callback<void(content::PermissionStatus)>& callback) {
  return -1;
}

void PermissionManager::UnsubscribePermissionStatusChange(int subscription_id) {
}

}  // namespace brightray
