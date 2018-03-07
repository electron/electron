// Copyright (c) 2016 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/atom_permission_manager.h"

#include <vector>

#include "atom/browser/web_contents_preferences.h"
#include "content/public/browser/child_process_security_policy.h"
#include "content/public/browser/permission_type.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/browser/render_view_host.h"
#include "content/public/browser/web_contents.h"

namespace atom {

namespace {

bool WebContentsDestroyed(int process_id) {
  auto contents =
      WebContentsPreferences::GetWebContentsFromProcessID(process_id);
  if (!contents)
    return true;
  return contents->IsBeingDestroyed();
}

void PermissionRequestResponseCallbackWrapper(
    const AtomPermissionManager::StatusCallback& callback,
    const std::vector<blink::mojom::PermissionStatus>& vector) {
  callback.Run(vector[0]);
}

}  // namespace

class AtomPermissionManager::PendingRequest {
 public:
  PendingRequest(content::RenderFrameHost* render_frame_host,
                 const std::vector<content::PermissionType>& permissions,
                 const StatusesCallback& callback)
      : render_process_id_(render_frame_host->GetProcess()->GetID()),
        callback_(callback),
        results_(permissions.size(), blink::mojom::PermissionStatus::DENIED),
        remaining_results_(permissions.size()) {}

  void SetPermissionStatus(int permission_id,
                           blink::mojom::PermissionStatus status) {
    DCHECK(!IsComplete());

    results_[permission_id] = status;
    --remaining_results_;
  }

  int render_process_id() const {
    return render_process_id_;
  }

  bool IsComplete() const {
    return remaining_results_ == 0;
  }

  void RunCallback() const {
    callback_.Run(results_);
  }

 private:
  int render_process_id_;
  const StatusesCallback callback_;
  std::vector<blink::mojom::PermissionStatus> results_;
  size_t remaining_results_;
};

AtomPermissionManager::AtomPermissionManager() {
}

AtomPermissionManager::~AtomPermissionManager() {
}

void AtomPermissionManager::SetPermissionRequestHandler(
    const RequestHandler& handler) {
  if (handler.is_null() && !pending_requests_.IsEmpty()) {
    for (PendingRequestsMap::const_iterator iter(&pending_requests_);
         !iter.IsAtEnd(); iter.Advance()) {
      auto request = iter.GetCurrentValue();
      if (!WebContentsDestroyed(request->render_process_id()))
        request->RunCallback();
    }
    pending_requests_.Clear();
  }
  request_handler_ = handler;
}

int AtomPermissionManager::RequestPermission(
    content::PermissionType permission,
    content::RenderFrameHost* render_frame_host,
    const GURL& requesting_origin,
    bool user_gesture,
    const StatusCallback& response_callback) {
  return RequestPermissionWithDetails(
      permission,
      render_frame_host,
      requesting_origin,
      user_gesture,
      nullptr,
      response_callback);
}

int AtomPermissionManager::RequestPermissionWithDetails(
    content::PermissionType permission,
    content::RenderFrameHost* render_frame_host,
    const GURL& requesting_origin,
    bool user_gesture,
    const base::DictionaryValue* details,
    const StatusCallback& response_callback) {
  return RequestPermissionsWithDetails(
      std::vector<content::PermissionType>(1, permission),
      render_frame_host,
      requesting_origin,
      user_gesture,
      details,
      base::Bind(&PermissionRequestResponseCallbackWrapper, response_callback));
}

int AtomPermissionManager::RequestPermissions(
    const std::vector<content::PermissionType>& permissions,
    content::RenderFrameHost* render_frame_host,
    const GURL& requesting_origin,
    bool user_gesture,
    const StatusesCallback& response_callback) {
  return RequestPermissionsWithDetails(
    permissions, render_frame_host, requesting_origin,
    user_gesture, nullptr, response_callback);
}

int AtomPermissionManager::RequestPermissionsWithDetails(
    const std::vector<content::PermissionType>& permissions,
    content::RenderFrameHost* render_frame_host,
    const GURL& requesting_origin,
    bool user_gesture,
    const base::DictionaryValue* details,
    const StatusesCallback& response_callback) {
  if (permissions.empty()) {
    response_callback.Run(std::vector<blink::mojom::PermissionStatus>());
    return kNoPendingOperation;
  }

  if (request_handler_.is_null()) {
    std::vector<blink::mojom::PermissionStatus> statuses;
    for (auto permission : permissions) {
      if (permission == content::PermissionType::MIDI_SYSEX) {
        content::ChildProcessSecurityPolicy::GetInstance()->
            GrantSendMidiSysExMessage(render_frame_host->GetProcess()->GetID());
      }
      statuses.push_back(blink::mojom::PermissionStatus::GRANTED);
    }
    response_callback.Run(statuses);
    return kNoPendingOperation;
  }

  auto web_contents =
      content::WebContents::FromRenderFrameHost(render_frame_host);
  int request_id = pending_requests_.Add(base::MakeUnique<PendingRequest>(
      render_frame_host, permissions, response_callback));

  for (size_t i = 0; i < permissions.size(); ++i) {
    auto permission = permissions[i];
    if (permission == content::PermissionType::MIDI_SYSEX) {
      content::ChildProcessSecurityPolicy::GetInstance()->
          GrantSendMidiSysExMessage(render_frame_host->GetProcess()->GetID());
    }
    const auto callback =
        base::Bind(&AtomPermissionManager::OnPermissionResponse,
                   base::Unretained(this), request_id, i);
    if (details == nullptr) {
      request_handler_.Run(web_contents, permission, callback,
                           base::DictionaryValue());
    } else {
      request_handler_.Run(web_contents, permission, callback, *details);
    }
  }

  return request_id;
}

void AtomPermissionManager::OnPermissionResponse(
    int request_id,
    int permission_id,
    blink::mojom::PermissionStatus status) {
  auto pending_request = pending_requests_.Lookup(request_id);
  if (!pending_request)
    return;

  pending_request->SetPermissionStatus(permission_id, status);
  if (pending_request->IsComplete()) {
    pending_request->RunCallback();
    pending_requests_.Remove(request_id);
  }
}

void AtomPermissionManager::CancelPermissionRequest(int request_id) {
  auto pending_request = pending_requests_.Lookup(request_id);
  if (!pending_request)
    return;

  if (!WebContentsDestroyed(pending_request->render_process_id()))
    pending_request->RunCallback();
  pending_requests_.Remove(request_id);
}

void AtomPermissionManager::ResetPermission(
    content::PermissionType permission,
    const GURL& requesting_origin,
    const GURL& embedding_origin) {
}

blink::mojom::PermissionStatus AtomPermissionManager::GetPermissionStatus(
    content::PermissionType permission,
    const GURL& requesting_origin,
    const GURL& embedding_origin) {
  return blink::mojom::PermissionStatus::GRANTED;
}

int AtomPermissionManager::SubscribePermissionStatusChange(
    content::PermissionType permission,
    const GURL& requesting_origin,
    const GURL& embedding_origin,
    const StatusCallback& callback) {
  return -1;
}

void AtomPermissionManager::UnsubscribePermissionStatusChange(
    int subscription_id) {
}

}  // namespace atom
