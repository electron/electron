// Copyright (c) 2016 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/electron_permission_manager.h"

#include <memory>
#include <utility>
#include <vector>

#include "content/public/browser/child_process_security_policy.h"
#include "content/public/browser/permission_controller.h"
#include "content/public/browser/permission_type.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/browser/render_view_host.h"
#include "content/public/browser/web_contents.h"
#include "shell/browser/electron_browser_client.h"
#include "shell/browser/electron_browser_main_parts.h"
#include "shell/browser/web_contents_preferences.h"

namespace electron {

namespace {

bool WebContentsDestroyed(int process_id) {
  content::WebContents* web_contents =
      static_cast<ElectronBrowserClient*>(ElectronBrowserClient::Get())
          ->GetWebContentsFromProcessID(process_id);
  if (!web_contents)
    return true;
  return web_contents->IsBeingDestroyed();
}

void PermissionRequestResponseCallbackWrapper(
    ElectronPermissionManager::StatusCallback callback,
    const std::vector<blink::mojom::PermissionStatus>& vector) {
  std::move(callback).Run(vector[0]);
}

}  // namespace

class ElectronPermissionManager::PendingRequest {
 public:
  PendingRequest(content::RenderFrameHost* render_frame_host,
                 const std::vector<content::PermissionType>& permissions,
                 StatusesCallback callback)
      : render_process_id_(render_frame_host->GetProcess()->GetID()),
        callback_(std::move(callback)),
        permissions_(permissions),
        results_(permissions.size(), blink::mojom::PermissionStatus::DENIED),
        remaining_results_(permissions.size()) {}

  void SetPermissionStatus(int permission_id,
                           blink::mojom::PermissionStatus status) {
    DCHECK(!IsComplete());

    if (status == blink::mojom::PermissionStatus::GRANTED) {
      const auto permission = permissions_[permission_id];
      if (permission == content::PermissionType::MIDI_SYSEX) {
        content::ChildProcessSecurityPolicy::GetInstance()
            ->GrantSendMidiSysExMessage(render_process_id_);
      } else if (permission == content::PermissionType::GEOLOCATION) {
        ElectronBrowserMainParts::Get()
            ->GetGeolocationControl()
            ->UserDidOptIntoLocationServices();
      }
    }

    results_[permission_id] = status;
    --remaining_results_;
  }

  int render_process_id() const { return render_process_id_; }

  bool IsComplete() const { return remaining_results_ == 0; }

  void RunCallback() {
    if (!callback_.is_null()) {
      std::move(callback_).Run(results_);
    }
  }

 private:
  int render_process_id_;
  StatusesCallback callback_;
  std::vector<content::PermissionType> permissions_;
  std::vector<blink::mojom::PermissionStatus> results_;
  size_t remaining_results_;
};

ElectronPermissionManager::ElectronPermissionManager() = default;

ElectronPermissionManager::~ElectronPermissionManager() = default;

void ElectronPermissionManager::SetPermissionRequestHandler(
    const RequestHandler& handler) {
  if (handler.is_null() && !pending_requests_.IsEmpty()) {
    for (PendingRequestsMap::iterator iter(&pending_requests_); !iter.IsAtEnd();
         iter.Advance()) {
      auto* request = iter.GetCurrentValue();
      if (!WebContentsDestroyed(request->render_process_id()))
        request->RunCallback();
    }
    pending_requests_.Clear();
  }
  request_handler_ = handler;
}

void ElectronPermissionManager::SetPermissionCheckHandler(
    const CheckHandler& handler) {
  check_handler_ = handler;
}

int ElectronPermissionManager::RequestPermission(
    content::PermissionType permission,
    content::RenderFrameHost* render_frame_host,
    const GURL& requesting_origin,
    bool user_gesture,
    StatusCallback response_callback) {
  return RequestPermissionWithDetails(permission, render_frame_host,
                                      requesting_origin, user_gesture, nullptr,
                                      std::move(response_callback));
}

int ElectronPermissionManager::RequestPermissionWithDetails(
    content::PermissionType permission,
    content::RenderFrameHost* render_frame_host,
    const GURL& requesting_origin,
    bool user_gesture,
    const base::DictionaryValue* details,
    StatusCallback response_callback) {
  return RequestPermissionsWithDetails(
      std::vector<content::PermissionType>(1, permission), render_frame_host,
      requesting_origin, user_gesture, details,
      base::BindOnce(PermissionRequestResponseCallbackWrapper,
                     std::move(response_callback)));
}

int ElectronPermissionManager::RequestPermissions(
    const std::vector<content::PermissionType>& permissions,
    content::RenderFrameHost* render_frame_host,
    const GURL& requesting_origin,
    bool user_gesture,
    StatusesCallback response_callback) {
  return RequestPermissionsWithDetails(permissions, render_frame_host,
                                       requesting_origin, user_gesture, nullptr,
                                       std::move(response_callback));
}

int ElectronPermissionManager::RequestPermissionsWithDetails(
    const std::vector<content::PermissionType>& permissions,
    content::RenderFrameHost* render_frame_host,
    const GURL& requesting_origin,
    bool user_gesture,
    const base::DictionaryValue* details,
    StatusesCallback response_callback) {
  if (permissions.empty()) {
    std::move(response_callback).Run({});
    return content::PermissionController::kNoPendingOperation;
  }

  if (request_handler_.is_null()) {
    std::vector<blink::mojom::PermissionStatus> statuses;
    for (auto permission : permissions) {
      if (permission == content::PermissionType::MIDI_SYSEX) {
        content::ChildProcessSecurityPolicy::GetInstance()
            ->GrantSendMidiSysExMessage(
                render_frame_host->GetProcess()->GetID());
      } else if (permission == content::PermissionType::GEOLOCATION) {
        ElectronBrowserMainParts::Get()
            ->GetGeolocationControl()
            ->UserDidOptIntoLocationServices();
      }
      statuses.push_back(blink::mojom::PermissionStatus::GRANTED);
    }
    std::move(response_callback).Run(statuses);
    return content::PermissionController::kNoPendingOperation;
  }

  auto* web_contents =
      content::WebContents::FromRenderFrameHost(render_frame_host);
  int request_id = pending_requests_.Add(std::make_unique<PendingRequest>(
      render_frame_host, permissions, std::move(response_callback)));

  for (size_t i = 0; i < permissions.size(); ++i) {
    auto permission = permissions[i];
    const auto callback =
        base::BindRepeating(&ElectronPermissionManager::OnPermissionResponse,
                            base::Unretained(this), request_id, i);
    auto mutable_details =
        details == nullptr ? base::DictionaryValue() : details->Clone();
    mutable_details.SetStringKey(
        "requestingUrl", render_frame_host->GetLastCommittedURL().spec());
    mutable_details.SetBoolKey("isMainFrame",
                               render_frame_host->GetParent() == nullptr);
    request_handler_.Run(web_contents, permission, callback, mutable_details);
  }

  return request_id;
}

void ElectronPermissionManager::OnPermissionResponse(
    int request_id,
    int permission_id,
    blink::mojom::PermissionStatus status) {
  auto* pending_request = pending_requests_.Lookup(request_id);
  if (!pending_request)
    return;

  pending_request->SetPermissionStatus(permission_id, status);
  if (pending_request->IsComplete()) {
    pending_request->RunCallback();
    pending_requests_.Remove(request_id);
  }
}

void ElectronPermissionManager::ResetPermission(
    content::PermissionType permission,
    const GURL& requesting_origin,
    const GURL& embedding_origin) {}

blink::mojom::PermissionStatus ElectronPermissionManager::GetPermissionStatus(
    content::PermissionType permission,
    const GURL& requesting_origin,
    const GURL& embedding_origin) {
  base::DictionaryValue details;
  details.SetString("embeddingOrigin", embedding_origin.spec());
  bool granted = CheckPermissionWithDetails(permission, nullptr,
                                            requesting_origin, &details);
  return granted ? blink::mojom::PermissionStatus::GRANTED
                 : blink::mojom::PermissionStatus::DENIED;
}

int ElectronPermissionManager::SubscribePermissionStatusChange(
    content::PermissionType permission,
    content::RenderFrameHost* render_frame_host,
    const GURL& requesting_origin,
    base::RepeatingCallback<void(blink::mojom::PermissionStatus)> callback) {
  return -1;
}

void ElectronPermissionManager::UnsubscribePermissionStatusChange(
    int subscription_id) {}

bool ElectronPermissionManager::CheckPermissionWithDetails(
    content::PermissionType permission,
    content::RenderFrameHost* render_frame_host,
    const GURL& requesting_origin,
    const base::DictionaryValue* details) const {
  if (check_handler_.is_null()) {
    return true;
  }
  auto* web_contents =
      render_frame_host
          ? content::WebContents::FromRenderFrameHost(render_frame_host)
          : nullptr;
  auto mutable_details =
      details == nullptr ? base::DictionaryValue() : details->Clone();
  if (render_frame_host) {
    mutable_details.SetStringKey(
        "requestingUrl", render_frame_host->GetLastCommittedURL().spec());
  }
  mutable_details.SetBoolKey(
      "isMainFrame",
      render_frame_host && render_frame_host->GetParent() == nullptr);
  switch (permission) {
    case content::PermissionType::AUDIO_CAPTURE:
      mutable_details.SetStringKey("mediaType", "audio");
      break;
    case content::PermissionType::VIDEO_CAPTURE:
      mutable_details.SetStringKey("mediaType", "video");
      break;
    default:
      break;
  }
  return check_handler_.Run(web_contents, permission, requesting_origin,
                            mutable_details);
}

blink::mojom::PermissionStatus
ElectronPermissionManager::GetPermissionStatusForFrame(
    content::PermissionType permission,
    content::RenderFrameHost* render_frame_host,
    const GURL& requesting_origin) {
  base::DictionaryValue details;
  bool granted = CheckPermissionWithDetails(permission, render_frame_host,
                                            requesting_origin, &details);
  return granted ? blink::mojom::PermissionStatus::GRANTED
                 : blink::mojom::PermissionStatus::DENIED;
}

}  // namespace electron
