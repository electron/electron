// Copyright (c) 2016 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/web_contents_permission_helper.h"

#include <memory>
#include <string>
#include <utility>

#include "content/public/browser/browser_context.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/browser/web_contents_user_data.h"
#include "shell/browser/electron_permission_manager.h"
#include "shell/browser/media/media_stream_devices_controller.h"

namespace {

std::string MediaStreamTypeToString(blink::mojom::MediaStreamType type) {
  switch (type) {
    case blink::mojom::MediaStreamType::DEVICE_AUDIO_CAPTURE:
      return "audio";
    case blink::mojom::MediaStreamType::DEVICE_VIDEO_CAPTURE:
      return "video";
    default:
      return "unknown";
  }
}

}  // namespace

namespace electron {

namespace {

void MediaAccessAllowed(const content::MediaStreamRequest& request,
                        content::MediaResponseCallback callback,
                        bool allowed) {
  MediaStreamDevicesController controller(request, std::move(callback));
  if (allowed)
    controller.TakeAction();
  else
    controller.Deny(blink::mojom::MediaStreamRequestResult::PERMISSION_DENIED);
}

void OnPermissionResponse(base::OnceCallback<void(bool)> callback,
                          blink::mojom::PermissionStatus status) {
  if (status == blink::mojom::PermissionStatus::GRANTED)
    std::move(callback).Run(true);
  else
    std::move(callback).Run(false);
}

}  // namespace

WebContentsPermissionHelper::WebContentsPermissionHelper(
    content::WebContents* web_contents)
    : content::WebContentsUserData<WebContentsPermissionHelper>(*web_contents),
      web_contents_(web_contents) {}

WebContentsPermissionHelper::~WebContentsPermissionHelper() = default;

void WebContentsPermissionHelper::RequestPermission(
    content::RenderFrameHost* requesting_frame,
    blink::PermissionType permission,
    base::OnceCallback<void(bool)> callback,
    bool user_gesture,
    const base::DictionaryValue* details) {
  auto* permission_manager = static_cast<ElectronPermissionManager*>(
      web_contents_->GetBrowserContext()->GetPermissionControllerDelegate());
  auto origin = web_contents_->GetLastCommittedURL();
  permission_manager->RequestPermissionWithDetails(
      permission, requesting_frame, origin, false, details,
      base::BindOnce(&OnPermissionResponse, std::move(callback)));
}

bool WebContentsPermissionHelper::CheckPermission(
    blink::PermissionType permission,
    const base::DictionaryValue* details) const {
  auto* rfh = web_contents_->GetMainFrame();
  auto* permission_manager = static_cast<ElectronPermissionManager*>(
      web_contents_->GetBrowserContext()->GetPermissionControllerDelegate());
  auto origin = web_contents_->GetLastCommittedURL();
  return permission_manager->CheckPermissionWithDetails(permission, rfh, origin,
                                                        details);
}

void WebContentsPermissionHelper::RequestFullscreenPermission(
    content::RenderFrameHost* requesting_frame,
    base::OnceCallback<void(bool)> callback) {
  RequestPermission(
      requesting_frame,
      static_cast<blink::PermissionType>(PermissionType::FULLSCREEN),
      std::move(callback));
}

void WebContentsPermissionHelper::RequestMediaAccessPermission(
    const content::MediaStreamRequest& request,
    content::MediaResponseCallback response_callback) {
  auto callback = base::BindOnce(&MediaAccessAllowed, request,
                                 std::move(response_callback));

  base::DictionaryValue details;
  auto media_types = std::make_unique<base::ListValue>();
  if (request.audio_type ==
      blink::mojom::MediaStreamType::DEVICE_AUDIO_CAPTURE) {
    media_types->Append("audio");
  }
  if (request.video_type ==
      blink::mojom::MediaStreamType::DEVICE_VIDEO_CAPTURE) {
    media_types->Append("video");
  }
  details.SetList("mediaTypes", std::move(media_types));
  details.SetString("securityOrigin", request.security_origin.spec());

  // The permission type doesn't matter here, AUDIO_CAPTURE/VIDEO_CAPTURE
  // are presented as same type in content_converter.h.
  RequestPermission(content::RenderFrameHost::FromID(request.render_process_id,
                                                     request.render_frame_id),
                    blink::PermissionType::AUDIO_CAPTURE, std::move(callback),
                    false, &details);
}

void WebContentsPermissionHelper::RequestWebNotificationPermission(
    content::RenderFrameHost* requesting_frame,
    base::OnceCallback<void(bool)> callback) {
  RequestPermission(requesting_frame, blink::PermissionType::NOTIFICATIONS,
                    std::move(callback));
}

void WebContentsPermissionHelper::RequestPointerLockPermission(
    bool user_gesture,
    bool last_unlocked_by_target,
    base::OnceCallback<void(content::WebContents*, bool, bool, bool)>
        callback) {
  RequestPermission(
      web_contents_->GetPrimaryMainFrame(),
      static_cast<blink::PermissionType>(PermissionType::POINTER_LOCK),
      base::BindOnce(std::move(callback), web_contents_, user_gesture,
                     last_unlocked_by_target),
      user_gesture);
}

void WebContentsPermissionHelper::RequestOpenExternalPermission(
    content::RenderFrameHost* requesting_frame,
    base::OnceCallback<void(bool)> callback,
    bool user_gesture,
    const GURL& url) {
  base::DictionaryValue details;
  details.SetString("externalURL", url.spec());
  RequestPermission(
      requesting_frame,
      static_cast<blink::PermissionType>(PermissionType::OPEN_EXTERNAL),
      std::move(callback), user_gesture, &details);
}

bool WebContentsPermissionHelper::CheckMediaAccessPermission(
    const GURL& security_origin,
    blink::mojom::MediaStreamType type) const {
  base::DictionaryValue details;
  details.SetString("securityOrigin", security_origin.spec());
  details.SetString("mediaType", MediaStreamTypeToString(type));
  // The permission type doesn't matter here, AUDIO_CAPTURE/VIDEO_CAPTURE
  // are presented as same type in content_converter.h.
  return CheckPermission(blink::PermissionType::AUDIO_CAPTURE, &details);
}

bool WebContentsPermissionHelper::CheckSerialAccessPermission(
    const url::Origin& embedding_origin) const {
  base::DictionaryValue details;
  details.SetString("securityOrigin", embedding_origin.GetURL().spec());
  return CheckPermission(
      static_cast<blink::PermissionType>(PermissionType::SERIAL), &details);
}

WEB_CONTENTS_USER_DATA_KEY_IMPL(WebContentsPermissionHelper);

}  // namespace electron
