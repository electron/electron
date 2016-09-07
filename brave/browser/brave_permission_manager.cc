// Copyright (c) 2016 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "brave/browser/brave_permission_manager.h"

#include "content/public/browser/child_process_security_policy.h"
#include "content/public/browser/permission_type.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/browser/web_contents.h"

namespace brave {

namespace {

bool WebContentsDestroyed(int render_process_id, int render_frame_id) {
  if (render_process_id == MSG_ROUTING_NONE)
    return false;

  auto host = content::RenderProcessHost::FromID(render_process_id);
  if (!host)
    return true;

  auto rfh =
      content::RenderFrameHost::FromID(render_process_id, render_frame_id);
  if (!rfh)
    return true;

  auto contents =
      content::WebContents::FromRenderFrameHost(rfh);
  if (!contents)
    return true;

  return contents->IsBeingDestroyed();
}

}  // namespace

BravePermissionManager::BravePermissionManager()
    : request_id_(0) {
}

BravePermissionManager::~BravePermissionManager() {
}

void BravePermissionManager::SetPermissionRequestHandler(
    const RequestHandler& handler) {
  if (handler.is_null() && !pending_requests_.empty()) {
    for (const auto& request : pending_requests_) {
      if (!WebContentsDestroyed(
          request.second.render_process_id, request.second.render_frame_id))
        request.second.callback.Run(blink::mojom::PermissionStatus::DENIED);
    }
    pending_requests_.clear();
  }
  request_handler_ = handler;
}

int BravePermissionManager::RequestPermission(
    content::PermissionType permission,
    content::RenderFrameHost* render_frame_host,
    const GURL& requesting_origin,
    const ResponseCallback& response_callback) {
  int render_frame_id = MSG_ROUTING_NONE;
  int render_process_id = MSG_ROUTING_NONE;
  GURL url;
  // web notifications do not currently have an available render_frame_host
  if (render_frame_host) {
    render_process_id = render_frame_host->GetProcess()->GetID();

    if (permission == content::PermissionType::MIDI_SYSEX) {
      content::ChildProcessSecurityPolicy::GetInstance()->
          GrantSendMidiSysExMessage(render_process_id);
    }

    content::WebContents* web_contents =
        content::WebContents::FromRenderFrameHost(render_frame_host);

    if (web_contents) {
      render_frame_id = render_frame_host->GetRoutingID();
      url = web_contents->GetURL();
    }
  }

  if (!request_handler_.is_null()) {
    ++request_id_;
    auto callback = base::Bind(&BravePermissionManager::OnPermissionResponse,
                               base::Unretained(this),
                               request_id_,
                               requesting_origin,
                               response_callback);

    pending_requests_[request_id_] =
        { render_process_id, render_frame_id, callback };

    request_handler_.Run(requesting_origin, url, permission, callback);
    return request_id_;
  }

  response_callback.Run(blink::mojom::PermissionStatus::GRANTED);
  return kNoPendingOperation;
}

void BravePermissionManager::OnPermissionResponse(
    int request_id,
    const GURL& origin,
    const ResponseCallback& callback,
    blink::mojom::PermissionStatus status) {
  auto request = pending_requests_.find(request_id);
  if (request != pending_requests_.end()) {
    if (!WebContentsDestroyed(
        request->second.render_process_id, request->second.render_frame_id))
      callback.Run(status);
    pending_requests_.erase(request);
  }
}

void BravePermissionManager::CancelPermissionRequest(int request_id) {
  auto request = pending_requests_.find(request_id);
  if (request != pending_requests_.end()) {
    if (!WebContentsDestroyed(
        request->second.render_process_id, request->second.render_frame_id)) {
      request->second.callback.Run(blink::mojom::PermissionStatus::DENIED);
    } else {
      pending_requests_.erase(request);
    }
  }
}

}  // namespace brave
