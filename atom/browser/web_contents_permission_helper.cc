// Copyright (c) 2016 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/web_contents_permission_helper.h"

#include <string>

#include "atom/browser/atom_permission_manager.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/media_capture_devices.h"
#include "content/public/browser/render_process_host.h"

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
    const content::MediaResponseCallback& callback,
    bool allowed) {
  if (!allowed) {
    callback.Run(content::MediaStreamDevices(),
                 content::MEDIA_DEVICE_PERMISSION_DENIED,
                 scoped_ptr<content::MediaStreamUI>());
    return;
  }

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

void OnPermissionResponse(const base::Callback<void(bool)>& callback,
                          content::PermissionStatus status) {
  if (status == content::PERMISSION_STATUS_GRANTED)
    callback.Run(true);
  else
    callback.Run(false);
}

}  // namespace

WebContentsPermissionHelper::WebContentsPermissionHelper(
    content::WebContents* web_contents)
    : web_contents_(web_contents) {
}

WebContentsPermissionHelper::~WebContentsPermissionHelper() {
}

void WebContentsPermissionHelper::RequestPermission(
    content::PermissionType permission,
    const base::Callback<void(bool)>& callback) {
  auto rfh = web_contents_->GetMainFrame();
  auto permission_manager = static_cast<AtomPermissionManager*>(
      web_contents_->GetBrowserContext()->GetPermissionManager());
  auto origin = web_contents_->GetLastCommittedURL();
  bool user_gesture = false;
  permission_manager->RequestPermission(
      permission, rfh, origin, user_gesture,
      base::Bind(&OnPermissionResponse, callback));
}

void WebContentsPermissionHelper::RequestMediaAccessPermission(
    const content::MediaStreamRequest& request,
    const content::MediaResponseCallback& response_callback) {
  auto callback = base::Bind(&MediaAccessAllowed, request, response_callback);
  // The permission type doesn't matter here, AUDIO_CAPTURE/VIDEO_CAPTURE
  // are presented as same type in content_converter.h.
  RequestPermission(content::PermissionType::AUDIO_CAPTURE, callback);
}

void WebContentsPermissionHelper::RequestWebNotificationPermission(
    const base::Callback<void(bool)>& callback) {
  RequestPermission(content::PermissionType::NOTIFICATIONS, callback);
}

}  // namespace atom
