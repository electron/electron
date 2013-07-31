// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE-CHROMIUM file.

#include "browser/media/media_stream_devices_controller.h"

#include "base/values.h"
#include "browser/media/media_capture_devices_dispatcher.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/common/media_stream_request.h"

namespace brightray {

namespace {

bool HasAnyAvailableDevice() {
  const content::MediaStreamDevices& audio_devices =
      MediaCaptureDevicesDispatcher::GetInstance()->GetAudioCaptureDevices();
  const content::MediaStreamDevices& video_devices =
      MediaCaptureDevicesDispatcher::GetInstance()->GetVideoCaptureDevices();

  return !audio_devices.empty() || !video_devices.empty();
};

}  // namespace

MediaStreamDevicesController::MediaStreamDevicesController(
    const content::MediaStreamRequest& request,
    const content::MediaResponseCallback& callback)
    : request_(request),
      callback_(callback),
      microphone_requested_(
          request_.audio_type == content::MEDIA_DEVICE_AUDIO_CAPTURE),
      webcam_requested_(
          request_.video_type == content::MEDIA_DEVICE_VIDEO_CAPTURE) {
}

MediaStreamDevicesController::~MediaStreamDevicesController() {}

bool MediaStreamDevicesController::TakeAction() {
  // Deny the request if there is no device attached to the OS.
  if (!HasAnyAvailableDevice()) {
    Deny();
    return true;
  }

  Accept();
  return true;
}

void MediaStreamDevicesController::Accept() {
  // Get the default devices for the request.
  content::MediaStreamDevices devices;
  if (microphone_requested_ || webcam_requested_) {
    switch (request_.request_type) {
      case content::MEDIA_OPEN_DEVICE:
        // For open device request pick the desired device or fall back to the
        // first available of the given type.
        MediaCaptureDevicesDispatcher::GetInstance()->GetRequestedDevice(
            request_.requested_device_id,
            microphone_requested_,
            webcam_requested_,
            &devices);
        break;
      case content::MEDIA_DEVICE_ACCESS:
      case content::MEDIA_GENERATE_STREAM:
      case content::MEDIA_ENUMERATE_DEVICES:
        // Get the default devices for the request.
        MediaCaptureDevicesDispatcher::GetInstance()->
            GetDefaultDevices(microphone_requested_,
                              webcam_requested_,
                              &devices);
        break;
    }
  }

  LOG(ERROR) << "Accept";
  callback_.Run(devices, scoped_ptr<content::MediaStreamUI>());
}

void MediaStreamDevicesController::Deny() {
  callback_.Run(content::MediaStreamDevices(),
                scoped_ptr<content::MediaStreamUI>());
}

}  // namespace brightray
