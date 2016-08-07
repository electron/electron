// Copyright (c) 2016 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#pragma once

#include "content/public/browser/permission_type.h"
#include "content/public/browser/web_contents_user_data.h"
#include "content/public/common/media_stream_request.h"

namespace atom {

// Applies the permission requested for WebContents.
class WebContentsPermissionHelper
    : public content::WebContentsUserData<WebContentsPermissionHelper> {
 public:
  ~WebContentsPermissionHelper() override;

  enum class PermissionType {
    POINTER_LOCK = static_cast<int>(content::PermissionType::NUM) + 1,
    FULLSCREEN,
    OPEN_EXTERNAL,
  };

  void RequestFullscreenPermission(
      const base::Callback<void(bool)>& callback);
  void RequestMediaAccessPermission(
      const content::MediaStreamRequest& request,
      const content::MediaResponseCallback& callback);
  void RequestWebNotificationPermission(
      const base::Callback<void(bool)>& callback);
  void RequestPointerLockPermission(bool user_gesture);
  void RequestOpenExternalPermission(
      const base::Callback<void(bool)>& callback,
      bool user_gesture);

 private:
  explicit WebContentsPermissionHelper(content::WebContents* web_contents);
  friend class content::WebContentsUserData<WebContentsPermissionHelper>;

  void RequestPermission(
      content::PermissionType permission,
      const base::Callback<void(bool)>& callback,
      bool user_gesture = false);

  content::WebContents* web_contents_;

  DISALLOW_COPY_AND_ASSIGN(WebContentsPermissionHelper);
};

}  // namespace atom
