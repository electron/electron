// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE-CHROMIUM file.

#ifndef BRIGHTRAY_BROWSER_MEDIA_MEDIA_STREAM_DEVICES_CONTROLLER_H_
#define BRIGHTRAY_BROWSER_MEDIA_MEDIA_STREAM_DEVICES_CONTROLLER_H_

#include "content/public/browser/web_contents_delegate.h"

namespace brightray {

class MediaStreamDevicesController {
 public:
  MediaStreamDevicesController(const content::MediaStreamRequest& request,
                               const content::MediaResponseCallback& callback);

  virtual ~MediaStreamDevicesController();

  // Public method to be called before creating the MediaStreamInfoBarDelegate.
  // This function will check the content settings exceptions and take the
  // corresponding action on exception which matches the request.
  bool TakeAction();

  // Public methods to be called by MediaStreamInfoBarDelegate;
  bool has_audio() const { return microphone_requested_; }
  bool has_video() const { return webcam_requested_; }
  void Accept();
  void Deny();

 private:
  // The original request for access to devices.
  const content::MediaStreamRequest request_;

  // The callback that needs to be Run to notify WebRTC of whether access to
  // audio/video devices was granted or not.
  content::MediaResponseCallback callback_;

  bool microphone_requested_;
  bool webcam_requested_;

  DISALLOW_COPY_AND_ASSIGN(MediaStreamDevicesController);
};

}  // namespace brightray

#endif  // BRIGHTRAY_BROWSER_MEDIA_MEDIA_STREAM_DEVICES_CONTROLLER_H_
