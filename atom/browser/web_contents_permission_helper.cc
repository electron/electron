// Copyright (c) 2016 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/web_contents_permission_helper.h"

#include <string>

#include "atom/browser/api/atom_api_web_contents.h"
#include "content/public/browser/media_capture_devices.h"

DEFINE_WEB_CONTENTS_USER_DATA_KEY(atom::WebContentsPermissionHelper);

namespace atom {

namespace {

const content::MediaStreamDevice* FindDeviceWithId(
    const content::MediaStreamDevices& devices,
    const std::string& device_id) {
  if (device_id.empty())
    return &(*devices.begin());
  for (const auto& iter : devices)
    if (iter.id == device_id)
      return &(iter);
  return nullptr;
}

void MediaAccessAllowed(
    const content::MediaStreamRequest& request,
    const content::MediaResponseCallback& callback) {
  content::MediaStreamDevices devices;
  content::MediaStreamRequestResult result = content::MEDIA_DEVICE_NO_HARDWARE;

  if (request.audio_type == content::MEDIA_DEVICE_AUDIO_CAPTURE) {
    const content::MediaStreamDevices& audio_devices =
        content::MediaCaptureDevices::GetInstance()->GetAudioCaptureDevices();
    const content::MediaStreamDevice* audio_device =
        FindDeviceWithId(audio_devices, request.requested_audio_device_id);
    if (audio_device)
      devices.push_back(*audio_device);
  }

  if (request.video_type == content::MEDIA_DEVICE_VIDEO_CAPTURE) {
    const content::MediaStreamDevices& video_devices =
        content::MediaCaptureDevices::GetInstance()->GetVideoCaptureDevices();
    const content::MediaStreamDevice* video_device =
        FindDeviceWithId(video_devices, request.requested_video_device_id);
    if (video_device)
      devices.push_back(*video_device);
  }

  if (!devices.empty())
    result = content::MEDIA_DEVICE_OK;

  callback.Run(devices, result, scoped_ptr<content::MediaStreamUI>());
}

}  // namespace

WebContentsPermissionHelper::WebContentsPermissionHelper(
    content::WebContents* web_contents,
    api::WebContents* api_web_contents)
    : api_web_contents_(api_web_contents) {
  web_contents->SetUserData(UserDataKey(), this);
}

WebContentsPermissionHelper::~WebContentsPermissionHelper() {
}

void WebContentsPermissionHelper::RequestMediaAccessPermission(
    const content::MediaStreamRequest& request,
    const content::MediaResponseCallback& callback) {
  if (api_web_contents_->IsGuest()) {
    const std::string& permission = "media";
    permission_map_[permission] = base::Bind(&MediaAccessAllowed,
                                             request,
                                             callback);
    api_web_contents_->Emit(
        "permission-request",
        permission,
        base::Bind(&WebContentsPermissionHelper::OnPermissionResponse,
                   base::Unretained(this), permission));
    return;
  }
  MediaAccessAllowed(request, callback);
}

void WebContentsPermissionHelper::RequestWebNotificationPermission(
    const base::Closure& callback) {
  if (api_web_contents_->IsGuest()) {
    const std::string& permission = "webNotification";
    permission_map_[permission] = callback;
    api_web_contents_->Emit(
        "permission-request",
        permission,
        base::Bind(&WebContentsPermissionHelper::OnPermissionResponse,
                   base::Unretained(this), permission));
    return;
  }
  callback.Run();
}

void WebContentsPermissionHelper::OnPermissionResponse(
    const std::string& permission, bool allowed) {
  auto it = permission_map_.find(permission);
  if (it != permission_map_.end()) {
    if (allowed)
      it->second.Run();
    permission_map_.erase(permission);
  }
}

}  // namespace atom
