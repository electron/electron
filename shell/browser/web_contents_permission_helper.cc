// Copyright (c) 2016 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/web_contents_permission_helper.h"

#include <memory>
#include <string>
#include <utility>

#include "content/public/browser/browser_context.h"
#include "content/public/browser/render_process_host.h"
#include "shell/browser/atom_permission_manager.h"
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

void OnPointerLockResponse(content::WebContents* web_contents, bool allowed) {
  if (web_contents)
    web_contents->GotResponseToLockMouseRequest(allowed);
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
    : web_contents_(web_contents) {}

WebContentsPermissionHelper::~WebContentsPermissionHelper() = default;

void WebContentsPermissionHelper::RequestPermission(
    content::PermissionType permission,
    base::OnceCallback<void(bool)> callback,
    bool user_gesture,
    const base::DictionaryValue* details) {
  auto* rfh = web_contents_->GetMainFrame();
  auto* permission_manager = static_cast<AtomPermissionManager*>(
      web_contents_->GetBrowserContext()->GetPermissionControllerDelegate());
  auto origin = web_contents_->GetLastCommittedURL();
  permission_manager->RequestPermissionWithDetails(
      permission, rfh, origin, false, details,
      base::BindOnce(&OnPermissionResponse, std::move(callback)));
}

bool WebContentsPermissionHelper::CheckPermission(
    content::PermissionType permission,
    const base::DictionaryValue* details) const {
  auto* rfh = web_contents_->GetMainFrame();
  auto* permission_manager = static_cast<AtomPermissionManager*>(
      web_contents_->GetBrowserContext()->GetPermissionControllerDelegate());
  auto origin = web_contents_->GetLastCommittedURL();
  return permission_manager->CheckPermissionWithDetails(permission, rfh, origin,
                                                        details);
}

void WebContentsPermissionHelper::RequestFullscreenPermission(
    base::OnceCallback<void(bool)> callback) {
  RequestPermission(
      static_cast<content::PermissionType>(PermissionType::FULLSCREEN),
      std::move(callback));
}

void WebContentsPermissionHelper::RequestMediaAccessPermission(
    const content::MediaStreamRequest& request,
    content::MediaResponseCallback response_callback) {
  auto callback = base::AdaptCallbackForRepeating(base::BindOnce(
      &MediaAccessAllowed, request, std::move(response_callback)));

  base::DictionaryValue details;
  std::unique_ptr<base::ListValue> media_types(new base::ListValue);
  if (request.audio_type ==
      blink::mojom::MediaStreamType::DEVICE_AUDIO_CAPTURE) {
    media_types->AppendString("audio");
  }
  if (request.video_type ==
      blink::mojom::MediaStreamType::DEVICE_VIDEO_CAPTURE) {
    media_types->AppendString("video");
  }
  details.SetList("mediaTypes", std::move(media_types));

  // The permission type doesn't matter here, AUDIO_CAPTURE/VIDEO_CAPTURE
  // are presented as same type in content_converter.h.
  RequestPermission(content::PermissionType::AUDIO_CAPTURE, std::move(callback),
                    false, &details);
}

void WebContentsPermissionHelper::RequestWebNotificationPermission(
    base::OnceCallback<void(bool)> callback) {
  RequestPermission(content::PermissionType::NOTIFICATIONS,
                    std::move(callback));
}

void WebContentsPermissionHelper::RequestPointerLockPermission(
    bool user_gesture) {
  RequestPermission(
      static_cast<content::PermissionType>(PermissionType::POINTER_LOCK),
      base::BindOnce(&OnPointerLockResponse, web_contents_), user_gesture);
}

void WebContentsPermissionHelper::RequestOpenExternalPermission(
    base::OnceCallback<void(bool)> callback,
    bool user_gesture,
    const GURL& url) {
  base::DictionaryValue details;
  details.SetString("externalURL", url.spec());
  RequestPermission(
      static_cast<content::PermissionType>(PermissionType::OPEN_EXTERNAL),
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
  return CheckPermission(content::PermissionType::AUDIO_CAPTURE, &details);
}

WEB_CONTENTS_USER_DATA_KEY_IMPL(WebContentsPermissionHelper)

}  // namespace electron
