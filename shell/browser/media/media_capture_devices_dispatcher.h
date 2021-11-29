// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE-CHROMIUM file.

#ifndef ELECTRON_SHELL_BROWSER_MEDIA_MEDIA_CAPTURE_DEVICES_DISPATCHER_H_
#define ELECTRON_SHELL_BROWSER_MEDIA_MEDIA_CAPTURE_DEVICES_DISPATCHER_H_

#include <string>

#include "base/memory/singleton.h"
#include "content/public/browser/media_observer.h"
#include "content/public/browser/media_stream_request.h"
#include "third_party/blink/public/common/mediastream/media_stream_request.h"

namespace electron {

// This singleton is used to receive updates about media events from the content
// layer.
class MediaCaptureDevicesDispatcher : public content::MediaObserver {
 public:
  static MediaCaptureDevicesDispatcher* GetInstance();

  // Methods for observers. Called on UI thread.
  const blink::MediaStreamDevices& GetAudioCaptureDevices();
  const blink::MediaStreamDevices& GetVideoCaptureDevices();

  // Helper to get the default devices which can be used by the media request.
  // Uses the first available devices if the default devices are not available.
  // If the return list is empty, it means there is no available device on the
  // OS.
  // Called on the UI thread.
  void GetDefaultDevices(bool audio,
                         bool video,
                         blink::MediaStreamDevices* devices);

  // Helpers for picking particular requested devices, identified by raw id.
  // If the device requested is not available it will return NULL.
  const blink::MediaStreamDevice* GetRequestedAudioDevice(
      const std::string& requested_audio_device_id);
  const blink::MediaStreamDevice* GetRequestedVideoDevice(
      const std::string& requested_video_device_id);

  // Returns the first available audio or video device, or NULL if no devices
  // are available.
  const blink::MediaStreamDevice* GetFirstAvailableAudioDevice();
  const blink::MediaStreamDevice* GetFirstAvailableVideoDevice();

  // Unittests that do not require actual device enumeration should call this
  // API on the singleton. It is safe to call this multiple times on the
  // singleton.
  void DisableDeviceEnumerationForTesting();

  // Overridden from content::MediaObserver:
  void OnAudioCaptureDevicesChanged() override;
  void OnVideoCaptureDevicesChanged() override;
  void OnMediaRequestStateChanged(int render_process_id,
                                  int render_view_id,
                                  int page_request_id,
                                  const GURL& security_origin,
                                  blink::mojom::MediaStreamType stream_type,
                                  content::MediaRequestState state) override;
  void OnCreatingAudioStream(int render_process_id,
                             int render_view_id) override;
  void OnSetCapturingLinkSecured(int render_process_id,
                                 int render_frame_id,
                                 int page_request_id,
                                 blink::mojom::MediaStreamType stream_type,
                                 bool is_secure) override;

  // disable copy
  MediaCaptureDevicesDispatcher(const MediaCaptureDevicesDispatcher&) = delete;
  MediaCaptureDevicesDispatcher& operator=(
      const MediaCaptureDevicesDispatcher&) = delete;

 private:
  friend struct base::DefaultSingletonTraits<MediaCaptureDevicesDispatcher>;

  MediaCaptureDevicesDispatcher();
  ~MediaCaptureDevicesDispatcher() override;

  // Only for testing, a list of cached audio capture devices.
  blink::MediaStreamDevices test_audio_devices_;

  // Only for testing, a list of cached video capture devices.
  blink::MediaStreamDevices test_video_devices_;

  // Flag used by unittests to disable device enumeration.
  bool is_device_enumeration_disabled_ = false;
};

}  // namespace electron

#endif  // ELECTRON_SHELL_BROWSER_MEDIA_MEDIA_CAPTURE_DEVICES_DISPATCHER_H_
