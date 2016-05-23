// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE-CHROMIUM file.

#ifndef BRIGHTRAY_BROWSER_MEDIA_MEDIA_CAPTURE_DEVICES_DISPATCHER_H_
#define BRIGHTRAY_BROWSER_MEDIA_MEDIA_CAPTURE_DEVICES_DISPATCHER_H_

#include "base/callback.h"
#include "base/memory/singleton.h"
#include "content/public/browser/media_observer.h"
#include "content/public/browser/web_contents_delegate.h"
#include "content/public/common/media_stream_request.h"

namespace brightray {

// This singleton is used to receive updates about media events from the content
// layer.
class MediaCaptureDevicesDispatcher : public content::MediaObserver {
 public:
  static MediaCaptureDevicesDispatcher* GetInstance();

  // Methods for observers. Called on UI thread.
  const content::MediaStreamDevices& GetAudioCaptureDevices();
  const content::MediaStreamDevices& GetVideoCaptureDevices();

  // Helper to get the default devices which can be used by the media request.
  // Uses the first available devices if the default devices are not available.
  // If the return list is empty, it means there is no available device on the
  // OS.
  // Called on the UI thread.
  void GetDefaultDevices(bool audio,
                         bool video,
                         content::MediaStreamDevices* devices);

  // Helpers for picking particular requested devices, identified by raw id.
  // If the device requested is not available it will return NULL.
  const content::MediaStreamDevice*
  GetRequestedAudioDevice(const std::string& requested_audio_device_id);
  const content::MediaStreamDevice*
  GetRequestedVideoDevice(const std::string& requested_video_device_id);

  // Returns the first available audio or video device, or NULL if no devices
  // are available.
  const content::MediaStreamDevice* GetFirstAvailableAudioDevice();
  const content::MediaStreamDevice* GetFirstAvailableVideoDevice();

  // Unittests that do not require actual device enumeration should call this
  // API on the singleton. It is safe to call this multiple times on the
  // signleton.
  void DisableDeviceEnumerationForTesting();

  // Overridden from content::MediaObserver:
  void OnAudioCaptureDevicesChanged() override;
  void OnVideoCaptureDevicesChanged() override;
  void OnMediaRequestStateChanged(
      int render_process_id,
      int render_view_id,
      int page_request_id,
      const GURL& security_origin,
      content::MediaStreamType stream_type,
      content::MediaRequestState state) override;
  void OnCreatingAudioStream(int render_process_id,
                             int render_view_id) override;

 private:
  friend struct base::DefaultSingletonTraits<MediaCaptureDevicesDispatcher>;

  MediaCaptureDevicesDispatcher();
  virtual ~MediaCaptureDevicesDispatcher();

  // Flag used by unittests to disable device enumeration.
  bool is_device_enumeration_disabled_;

  DISALLOW_COPY_AND_ASSIGN(MediaCaptureDevicesDispatcher);
};

}  // namespace brightray

#endif  // BRIGHTRAY_BROWSER_MEDIA_MEDIA_CAPTURE_DEVICES_DISPATCHER_H_
