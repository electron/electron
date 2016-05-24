// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE-CHROMIUM file.

#include "browser/media/media_stream_devices_controller.h"

#include "browser/media/media_capture_devices_dispatcher.h"

#include "content/public/browser/desktop_media_id.h"
#include "content/public/common/media_stream_request.h"

namespace brightray {

namespace {

bool HasAnyAvailableDevice() {
  const content::MediaStreamDevices& audio_devices =
      MediaCaptureDevicesDispatcher::GetInstance()->GetAudioCaptureDevices();
  const content::MediaStreamDevices& video_devices =
      MediaCaptureDevicesDispatcher::GetInstance()->GetVideoCaptureDevices();

  return !audio_devices.empty() || !video_devices.empty();
}

}  // namespace

MediaStreamDevicesController::MediaStreamDevicesController(
    const content::MediaStreamRequest& request,
    const content::MediaResponseCallback& callback)
    : request_(request),
      callback_(callback),
      // For MEDIA_OPEN_DEVICE requests (Pepper) we always request both webcam
      // and microphone to avoid popping two infobars.
      microphone_requested_(
          request.audio_type == content::MEDIA_DEVICE_AUDIO_CAPTURE ||
          request.request_type == content::MEDIA_OPEN_DEVICE_PEPPER_ONLY),
      webcam_requested_(
          request.video_type == content::MEDIA_DEVICE_VIDEO_CAPTURE ||
          request.request_type == content::MEDIA_OPEN_DEVICE_PEPPER_ONLY) {
}

MediaStreamDevicesController::~MediaStreamDevicesController() {
  if (!callback_.is_null()) {
    callback_.Run(content::MediaStreamDevices(),
                  content::MEDIA_DEVICE_INVALID_STATE,
                  std::unique_ptr<content::MediaStreamUI>());
  }
}

bool MediaStreamDevicesController::TakeAction() {
  // Do special handling of desktop screen cast.
  if (request_.audio_type == content::MEDIA_TAB_AUDIO_CAPTURE ||
      request_.video_type == content::MEDIA_TAB_VIDEO_CAPTURE ||
      request_.audio_type == content::MEDIA_DESKTOP_AUDIO_CAPTURE ||
      request_.video_type == content::MEDIA_DESKTOP_VIDEO_CAPTURE) {
    HandleUserMediaRequest();
    return true;
  }

  // Deny the request if there is no device attached to the OS.
  if (!HasAnyAvailableDevice()) {
    Deny(content::MEDIA_DEVICE_NO_HARDWARE);
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
      case content::MEDIA_OPEN_DEVICE_PEPPER_ONLY: {
        const content::MediaStreamDevice* device = NULL;
        // For open device request pick the desired device or fall back to the
        // first available of the given type.
        if (request_.audio_type == content::MEDIA_DEVICE_AUDIO_CAPTURE) {
          device = MediaCaptureDevicesDispatcher::GetInstance()->
              GetRequestedAudioDevice(request_.requested_audio_device_id);
          // TODO(wjia): Confirm this is the intended behavior.
          if (!device) {
            device = MediaCaptureDevicesDispatcher::GetInstance()->
                GetFirstAvailableAudioDevice();
          }
        } else if (request_.video_type == content::MEDIA_DEVICE_VIDEO_CAPTURE) {
          // Pepper API opens only one device at a time.
          device = MediaCaptureDevicesDispatcher::GetInstance()->
              GetRequestedVideoDevice(request_.requested_video_device_id);
          // TODO(wjia): Confirm this is the intended behavior.
          if (!device) {
            device = MediaCaptureDevicesDispatcher::GetInstance()->
                GetFirstAvailableVideoDevice();
          }
        }
        if (device)
          devices.push_back(*device);
        break;
      } case content::MEDIA_GENERATE_STREAM: {
        bool needs_audio_device = microphone_requested_;
        bool needs_video_device = webcam_requested_;

        // Get the exact audio or video device if an id is specified.
        if (!request_.requested_audio_device_id.empty()) {
          const content::MediaStreamDevice* audio_device =
              MediaCaptureDevicesDispatcher::GetInstance()->
                  GetRequestedAudioDevice(request_.requested_audio_device_id);
          if (audio_device) {
            devices.push_back(*audio_device);
            needs_audio_device = false;
          }
        }
        if (!request_.requested_video_device_id.empty()) {
          const content::MediaStreamDevice* video_device =
              MediaCaptureDevicesDispatcher::GetInstance()->
                  GetRequestedVideoDevice(request_.requested_video_device_id);
          if (video_device) {
            devices.push_back(*video_device);
            needs_video_device = false;
          }
        }

        // If either or both audio and video devices were requested but not
        // specified by id, get the default devices.
        if (needs_audio_device || needs_video_device) {
          MediaCaptureDevicesDispatcher::GetInstance()->
              GetDefaultDevices(needs_audio_device,
                                needs_video_device,
                                &devices);
        }
        break;
      } case content::MEDIA_DEVICE_ACCESS:
        // Get the default devices for the request.
        MediaCaptureDevicesDispatcher::GetInstance()->
            GetDefaultDevices(microphone_requested_,
                              webcam_requested_,
                              &devices);
        break;
      case content::MEDIA_ENUMERATE_DEVICES:
        // Do nothing.
        NOTREACHED();
        break;
    }
  }

  content::MediaResponseCallback cb = callback_;
  callback_.Reset();
  cb.Run(devices, content::MEDIA_DEVICE_OK, std::unique_ptr<content::MediaStreamUI>());
}

void MediaStreamDevicesController::Deny(content::MediaStreamRequestResult result) {
  content::MediaResponseCallback cb = callback_;
  callback_.Reset();
  cb.Run(content::MediaStreamDevices(),
         result,
         std::unique_ptr<content::MediaStreamUI>());
}

void MediaStreamDevicesController::HandleUserMediaRequest() {
  content::MediaStreamDevices devices;

  if (request_.audio_type == content::MEDIA_TAB_AUDIO_CAPTURE) {
    devices.push_back(content::MediaStreamDevice(
        content::MEDIA_TAB_AUDIO_CAPTURE, "", ""));
  }
  if (request_.video_type == content::MEDIA_TAB_VIDEO_CAPTURE) {
    devices.push_back(content::MediaStreamDevice(
        content::MEDIA_TAB_VIDEO_CAPTURE, "", ""));
  }
  if (request_.audio_type == content::MEDIA_DESKTOP_AUDIO_CAPTURE) {
    devices.push_back(content::MediaStreamDevice(
        content::MEDIA_DESKTOP_AUDIO_CAPTURE, "loopback", "System Audio"));
  }
  if (request_.video_type == content::MEDIA_DESKTOP_VIDEO_CAPTURE) {
    content::DesktopMediaID screen_id;
    // If the device id wasn't specified then this is a screen capture request
    // (i.e. chooseDesktopMedia() API wasn't used to generate device id).
    if (request_.requested_video_device_id.empty()) {
      screen_id = content::DesktopMediaID(content::DesktopMediaID::TYPE_SCREEN,
                                          -1  /* kFullDesktopScreenId */);
    } else {
      screen_id =
          content::DesktopMediaID::Parse(request_.requested_video_device_id);
    }

    devices.push_back(
        content::MediaStreamDevice(content::MEDIA_DESKTOP_VIDEO_CAPTURE,
                                   screen_id.ToString(), "Screen"));
  }

  content::MediaResponseCallback cb = callback_;
  callback_.Reset();
  cb.Run(devices,
         devices.empty() ? content::MEDIA_DEVICE_INVALID_STATE :
                           content::MEDIA_DEVICE_OK,
         std::unique_ptr<content::MediaStreamUI>());
}

}  // namespace brightray
