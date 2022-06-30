// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE-CHROMIUM file.

#include "shell/browser/media/media_stream_devices_controller.h"

#include <memory>
#include <utility>

#include "base/callback_helpers.h"
#include "chrome/browser/media/webrtc/desktop_media_list.h"
#include "chrome/browser/media/webrtc/native_desktop_media_list.h"
#include "content/public/browser/desktop_capture.h"
#include "content/public/browser/desktop_media_id.h"
#include "content/public/browser/media_stream_request.h"
#include "shell/browser/electron_browser_context.h"
#include "shell/browser/media/media_capture_devices_dispatcher.h"
#include "third_party/blink/public/mojom/mediastream/media_stream.mojom.h"
#include "third_party/webrtc/modules/desktop_capture/desktop_capturer.h"

namespace electron {

namespace {

bool HasAnyAvailableDevice() {
  const blink::MediaStreamDevices& audio_devices =
      MediaCaptureDevicesDispatcher::GetInstance()->GetAudioCaptureDevices();
  const blink::MediaStreamDevices& video_devices =
      MediaCaptureDevicesDispatcher::GetInstance()->GetVideoCaptureDevices();

  return !(audio_devices.empty() && video_devices.empty());
}

}  // namespace

MediaStreamDevicesController::MediaStreamDevicesController(
    const content::MediaStreamRequest& request,
    content::MediaResponseCallback callback)
    : request_(request),
      callback_(std::move(callback)),
      // For MEDIA_OPEN_DEVICE requests (Pepper) we always request both webcam
      // and microphone to avoid popping two infobars.
      microphone_requested_(
          request.audio_type ==
              blink::mojom::MediaStreamType::DEVICE_AUDIO_CAPTURE ||
          request.request_type == blink::MEDIA_OPEN_DEVICE_PEPPER_ONLY),
      webcam_requested_(
          request.video_type ==
              blink::mojom::MediaStreamType::DEVICE_VIDEO_CAPTURE ||
          request.request_type == blink::MEDIA_OPEN_DEVICE_PEPPER_ONLY) {}

MediaStreamDevicesController::~MediaStreamDevicesController() {
  if (!callback_.is_null()) {
    std::move(callback_).Run(
        blink::mojom::StreamDevicesSet(),
        blink::mojom::MediaStreamRequestResult::FAILED_DUE_TO_SHUTDOWN,
        std::unique_ptr<content::MediaStreamUI>());
  }
}

void MediaStreamDevicesController::TakeAction() {
  // Do special handling of desktop screen cast.
  if (request_.audio_type ==
          blink::mojom::MediaStreamType::GUM_TAB_AUDIO_CAPTURE ||
      request_.video_type ==
          blink::mojom::MediaStreamType::GUM_TAB_VIDEO_CAPTURE ||
      request_.audio_type ==
          blink::mojom::MediaStreamType::GUM_DESKTOP_AUDIO_CAPTURE ||
      request_.video_type ==
          blink::mojom::MediaStreamType::GUM_DESKTOP_VIDEO_CAPTURE) {
    HandleUserMediaRequest();
    return;
  }

  content::RenderFrameHost* rfh = content::RenderFrameHost::FromID(
      request_.render_process_id, request_.render_frame_id);
  if (!rfh)
    return;

  content::BrowserContext* browser_context = rfh->GetBrowserContext();
  ElectronBrowserContext* electron_browser_context =
      static_cast<ElectronBrowserContext*>(browser_context);
  auto split_callback = base::SplitOnceCallback(std::move(callback_));
  if (electron_browser_context->ChooseMediaDevice(
          request_, std::move(split_callback.second)))
    return;
  callback_ = std::move(split_callback.first);

  // Deny the request if there is no device attached to the OS.
  if (HasAnyAvailableDevice())
    Accept();
  else
    Deny(blink::mojom::MediaStreamRequestResult::NO_HARDWARE);
}

void MediaStreamDevicesController::Accept() {
  // Get the default devices for the request.
  blink::mojom::StreamDevicesSetPtr stream_devices_set =
      blink::mojom::StreamDevicesSet::New();
  // TODO(crbug.com/1300883): Generalize to multiple streams.
  stream_devices_set->stream_devices.emplace_back(
      blink::mojom::StreamDevices::New());
  blink::mojom::StreamDevices& stream_devices =
      *stream_devices_set->stream_devices[0];
  if (microphone_requested_ || webcam_requested_) {
    switch (request_.request_type) {
      case blink::MEDIA_OPEN_DEVICE_PEPPER_ONLY: {
        const blink::MediaStreamDevice* device = nullptr;
        // For open device request pick the desired device or fall back to the
        // first available of the given type.
        if (request_.audio_type ==
            blink::mojom::MediaStreamType::DEVICE_AUDIO_CAPTURE) {
          device =
              MediaCaptureDevicesDispatcher::GetInstance()
                  ->GetRequestedAudioDevice(request_.requested_audio_device_id);
          // TODO(wjia): Confirm this is the intended behavior.
          if (!device) {
            device = MediaCaptureDevicesDispatcher::GetInstance()
                         ->GetFirstAvailableAudioDevice();
          }
          if (device)
            stream_devices.audio_device = *device;
        } else if (request_.video_type ==
                   blink::mojom::MediaStreamType::DEVICE_VIDEO_CAPTURE) {
          // Pepper API opens only one device at a time.
          device =
              MediaCaptureDevicesDispatcher::GetInstance()
                  ->GetRequestedVideoDevice(request_.requested_video_device_id);
          // TODO(wjia): Confirm this is the intended behavior.
          if (!device) {
            device = MediaCaptureDevicesDispatcher::GetInstance()
                         ->GetFirstAvailableVideoDevice();
          }
          if (device)
            stream_devices.video_device = *device;
        }
        break;
      }
      case blink::MEDIA_GENERATE_STREAM: {
        bool needs_audio_device = microphone_requested_;
        bool needs_video_device = webcam_requested_;

        // Get the exact audio or video device if an id is specified.
        if (!request_.requested_audio_device_id.empty()) {
          const blink::MediaStreamDevice* audio_device =
              MediaCaptureDevicesDispatcher::GetInstance()
                  ->GetRequestedAudioDevice(request_.requested_audio_device_id);
          if (audio_device) {
            stream_devices.audio_device = *audio_device;
            needs_audio_device = false;
          }
        }
        if (!request_.requested_video_device_id.empty()) {
          const blink::MediaStreamDevice* video_device =
              MediaCaptureDevicesDispatcher::GetInstance()
                  ->GetRequestedVideoDevice(request_.requested_video_device_id);
          if (video_device) {
            stream_devices.video_device = *video_device;
            needs_video_device = false;
          }
        }

        // If either or both audio and video devices were requested but not
        // specified by id, get the default devices.
        if (needs_audio_device || needs_video_device) {
          MediaCaptureDevicesDispatcher::GetInstance()->GetDefaultDevices(
              needs_audio_device, needs_video_device, stream_devices);
        }
        break;
      }
      case blink::MEDIA_DEVICE_ACCESS: {
        // Get the default devices for the request.
        MediaCaptureDevicesDispatcher::GetInstance()->GetDefaultDevices(
            microphone_requested_, webcam_requested_, stream_devices);
        break;
      }
      case blink::MEDIA_DEVICE_UPDATE: {
        NOTREACHED();
        break;
      }
      case blink::MEDIA_GET_OPEN_DEVICE: {
        // Transferred tracks, that use blink::MEDIA_GET_OPEN_DEVICE type, do
        // not need to get permissions for blink::mojom::StreamDevices as those
        // are controlled by the original context.
        NOTREACHED();
        break;
      }
    }
  }

  std::move(callback_).Run(*stream_devices_set,
                           blink::mojom::MediaStreamRequestResult::OK,
                           std::unique_ptr<content::MediaStreamUI>());
}

void MediaStreamDevicesController::Deny(
    blink::mojom::MediaStreamRequestResult result) {
  std::move(callback_).Run(blink::mojom::StreamDevicesSet(), result,
                           std::unique_ptr<content::MediaStreamUI>());
}

void MediaStreamDevicesController::HandleUserMediaRequest() {
  // Get the default devices for the request.
  blink::mojom::StreamDevicesSetPtr stream_devices_set =
      blink::mojom::StreamDevicesSet::New();

  // TODO(crbug.com/1300883): Generalize to multiple streams.
  stream_devices_set->stream_devices.emplace_back(
      blink::mojom::StreamDevices::New());
  blink::mojom::StreamDevices& devices = *stream_devices_set->stream_devices[0];

  if (request_.audio_type ==
      blink::mojom::MediaStreamType::GUM_TAB_AUDIO_CAPTURE) {
    devices.audio_device = blink::MediaStreamDevice(
        blink::mojom::MediaStreamType::GUM_TAB_AUDIO_CAPTURE, "", "");
  }
  if (request_.video_type ==
      blink::mojom::MediaStreamType::GUM_TAB_VIDEO_CAPTURE) {
    devices.video_device = blink::MediaStreamDevice(
        blink::mojom::MediaStreamType::GUM_TAB_VIDEO_CAPTURE, "", "");
  }
  if (request_.audio_type ==
      blink::mojom::MediaStreamType::GUM_DESKTOP_AUDIO_CAPTURE) {
    devices.audio_device = blink::MediaStreamDevice(
        blink::mojom::MediaStreamType::GUM_DESKTOP_AUDIO_CAPTURE, "loopback",
        "System Audio");
  }
  if (request_.video_type ==
      blink::mojom::MediaStreamType::GUM_DESKTOP_VIDEO_CAPTURE) {
    content::DesktopMediaID screen_id;
    // If the device id wasn't specified then this is a screen capture request
    // (i.e. chooseDesktopMedia() API wasn't used to generate device id).
    if (request_.requested_video_device_id.empty()) {
      screen_id = content::DesktopMediaID(content::DesktopMediaID::TYPE_SCREEN,
                                          -1 /* kFullDesktopScreenId */);
    } else {
      screen_id =
          content::DesktopMediaID::Parse(request_.requested_video_device_id);
    }

    devices.video_device = blink::MediaStreamDevice(
        blink::mojom::MediaStreamType::GUM_DESKTOP_VIDEO_CAPTURE,
        screen_id.ToString(), "Screen");
  }

  bool empty =
      !devices.audio_device.has_value() && !devices.video_device.has_value();
  std::move(callback_).Run(
      *stream_devices_set,
      empty ? blink::mojom::MediaStreamRequestResult::NO_HARDWARE
            : blink::mojom::MediaStreamRequestResult::OK,
      std::unique_ptr<content::MediaStreamUI>());
}

}  // namespace electron
