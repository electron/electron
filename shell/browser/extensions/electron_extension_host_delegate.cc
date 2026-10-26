// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "shell/browser/extensions/electron_extension_host_delegate.h"

#include <algorithm>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/check.h"
#include "content/public/browser/media_capture_devices.h"
#include "extensions/common/extension.h"
#include "extensions/common/permissions/permissions_data.h"
#include "shell/browser/api/electron_api_web_contents.h"
#include "shell/browser/extensions/electron_extension_web_contents_observer.h"
#include "shell/common/gin_helper/handle.h"
#include "third_party/blink/public/mojom/mediastream/media_stream.mojom.h"
#include "v8/include/v8.h"

using blink::MediaStreamDevice;
using blink::MediaStreamDevices;
using content::MediaCaptureDevices;
using extensions::mojom::APIPermissionID;

namespace extensions {

// These functions are copied from the now deleted upstream implementations
// https://source.chromium.org/chromium/chromium/src/+/main:extensions/browser/media_capture_util.cc;drc=b9ff62bb6ff2143fc168dff0e3392e4335de1b8a
namespace {

const MediaStreamDevice* GetRequestedDeviceOrDefault(
    const MediaStreamDevices& devices,
    const std::vector<std::string>& requested_device_ids) {
  for (const auto& requested_device_id : requested_device_ids) {
    if (requested_device_id.empty())
      continue;
    auto it =
        std::ranges::find(devices, requested_device_id, &MediaStreamDevice::id);
    if (it != devices.end())
      return &(*it);
  }
  if (!devices.empty())
    return &devices[0];
  return nullptr;
}

void VerifyMediaAccessPermission(blink::mojom::MediaStreamType type,
                                 const Extension* extension) {
  const PermissionsData* permissions_data = extension->permissions_data();
  if (type == blink::mojom::MediaStreamType::DEVICE_AUDIO_CAPTURE) {
    CHECK(permissions_data->HasAPIPermission(APIPermissionID::kAudioCapture))
        << "Audio capture request but no audioCapture permission in manifest.";
  } else {
    DCHECK(type == blink::mojom::MediaStreamType::DEVICE_VIDEO_CAPTURE);
    CHECK(permissions_data->HasAPIPermission(APIPermissionID::kVideoCapture))
        << "Video capture request but no videoCapture permission in manifest.";
  }
}

void GrantMediaStreamRequest(content::WebContents* web_contents,
                             const content::MediaStreamRequest& request,
                             content::MediaResponseCallback callback,
                             const Extension* extension) {
  DCHECK(request.audio_type ==
             blink::mojom::MediaStreamType::DEVICE_AUDIO_CAPTURE ||
         request.video_type ==
             blink::mojom::MediaStreamType::DEVICE_VIDEO_CAPTURE);

  blink::mojom::StreamDevicesSet stream_devices_set;
  stream_devices_set.stream_devices.emplace_back(
      blink::mojom::StreamDevices::New());
  blink::mojom::StreamDevices& devices = *stream_devices_set.stream_devices[0];

  if (request.audio_type ==
      blink::mojom::MediaStreamType::DEVICE_AUDIO_CAPTURE) {
    VerifyMediaAccessPermission(request.audio_type, extension);
    const MediaStreamDevice* device = GetRequestedDeviceOrDefault(
        MediaCaptureDevices::GetInstance()->GetAudioCaptureDevices(),
        request.requested_audio_device_ids);
    if (device)
      devices.audio_device = *device;
  }

  if (request.video_type ==
      blink::mojom::MediaStreamType::DEVICE_VIDEO_CAPTURE) {
    VerifyMediaAccessPermission(request.video_type, extension);
    const MediaStreamDevice* device = GetRequestedDeviceOrDefault(
        MediaCaptureDevices::GetInstance()->GetVideoCaptureDevices(),
        request.requested_video_device_ids);
    if (device)
      devices.video_device = *device;
  }

  std::unique_ptr<content::MediaStreamUI> ui;
  std::move(callback).Run(
      stream_devices_set,
      (devices.audio_device.has_value() || devices.video_device.has_value())
          ? blink::mojom::MediaStreamRequestResult::OK
          : blink::mojom::MediaStreamRequestResult::INVALID_STATE,
      std::move(ui));
}

}  // namespace

ElectronExtensionHostDelegate::ElectronExtensionHostDelegate() = default;

ElectronExtensionHostDelegate::~ElectronExtensionHostDelegate() = default;

void ElectronExtensionHostDelegate::OnExtensionHostCreated(
    content::WebContents* web_contents) {
  ElectronExtensionWebContentsObserver::CreateForWebContents(web_contents);
  v8::Isolate* isolate = v8::Isolate::GetCurrent();
  v8::HandleScope scope(isolate);
  electron::api::WebContents::FromOrCreate(isolate, web_contents);
}

void ElectronExtensionHostDelegate::CreateTab(
    std::unique_ptr<content::WebContents> web_contents,
    const GURL& target_url,
    const ExtensionId& extension_id,
    WindowOpenDisposition disposition,
    const blink::mojom::WindowFeatures& window_features,
    bool user_gesture) {
  // TODO(jamescook): Should app_shell support opening popup windows?
  NOTREACHED();
}

void ElectronExtensionHostDelegate::ProcessMediaAccessRequest(
    content::WebContents* web_contents,
    const content::MediaStreamRequest& request,
    content::MediaResponseCallback callback,
    const Extension* extension) {
  // Allow access to the microphone and/or camera.
  GrantMediaStreamRequest(web_contents, request, std::move(callback),
                          extension);
}

bool ElectronExtensionHostDelegate::CheckMediaAccessPermission(
    content::RenderFrameHost* render_frame_host,
    const url::Origin& security_origin,
    blink::mojom::MediaStreamType type,
    const Extension* extension) {
  VerifyMediaAccessPermission(type, extension);
  return true;
}

content::PictureInPictureResult
ElectronExtensionHostDelegate::EnterPictureInPicture(
    content::WebContents* web_contents) {
  NOTREACHED();
}

void ElectronExtensionHostDelegate::ExitPictureInPicture() {
  NOTREACHED();
}

}  // namespace extensions
