// Copyright (c) 2016 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef SHELL_BROWSER_WEB_CONTENTS_PERMISSION_HELPER_H_
#define SHELL_BROWSER_WEB_CONTENTS_PERMISSION_HELPER_H_

#include "content/public/browser/media_stream_request.h"
#include "content/public/browser/permission_type.h"
#include "content/public/browser/web_contents_user_data.h"
#include "third_party/blink/public/common/mediastream/media_stream_request.h"

namespace electron {

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

  // Asynchronous Requests
  void RequestFullscreenPermission(base::OnceCallback<void(bool)> callback);
  void RequestMediaAccessPermission(const content::MediaStreamRequest& request,
                                    content::MediaResponseCallback callback);
  void RequestWebNotificationPermission(
      base::OnceCallback<void(bool)> callback);
  void RequestPointerLockPermission(bool user_gesture);
  void RequestOpenExternalPermission(base::OnceCallback<void(bool)> callback,
                                     bool user_gesture,
                                     const GURL& url);

  // Synchronous Checks
  bool CheckMediaAccessPermission(const GURL& security_origin,
                                  blink::mojom::MediaStreamType type) const;

 private:
  explicit WebContentsPermissionHelper(content::WebContents* web_contents);
  friend class content::WebContentsUserData<WebContentsPermissionHelper>;

  void RequestPermission(content::PermissionType permission,
                         base::OnceCallback<void(bool)> callback,
                         bool user_gesture = false,
                         const base::DictionaryValue* details = nullptr);

  bool CheckPermission(content::PermissionType permission,
                       const base::DictionaryValue* details) const;

  content::WebContents* web_contents_;

  WEB_CONTENTS_USER_DATA_KEY_DECL();

  DISALLOW_COPY_AND_ASSIGN(WebContentsPermissionHelper);
};

}  // namespace electron

#endif  // SHELL_BROWSER_WEB_CONTENTS_PERMISSION_HELPER_H_
