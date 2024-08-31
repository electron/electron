// Copyright (c) 2016 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_WEB_CONTENTS_PERMISSION_HELPER_H_
#define ELECTRON_SHELL_BROWSER_WEB_CONTENTS_PERMISSION_HELPER_H_

#include "base/memory/raw_ptr.h"
#include "base/values.h"
#include "content/public/browser/media_stream_request.h"
#include "content/public/browser/web_contents_user_data.h"
#include "third_party/blink/public/common/mediastream/media_stream_request.h"
#include "third_party/blink/public/common/permissions/permission_utils.h"

namespace electron {

// Applies the permission requested for WebContents.
class WebContentsPermissionHelper
    : public content::WebContentsUserData<WebContentsPermissionHelper> {
 public:
  ~WebContentsPermissionHelper() override;

  // disable copy
  WebContentsPermissionHelper(const WebContentsPermissionHelper&) = delete;
  WebContentsPermissionHelper& operator=(const WebContentsPermissionHelper&) =
      delete;

  enum class PermissionType {
    FULLSCREEN = static_cast<int>(blink::PermissionType::NUM) + 1,
    OPEN_EXTERNAL,
    SERIAL,
    HID,
    USB,
    KEYBOARD_LOCK,
    FILE_SYSTEM
  };

  // Asynchronous Requests
  void RequestFullscreenPermission(content::RenderFrameHost* requesting_frame,
                                   base::OnceCallback<void(bool)> callback);
  void RequestMediaAccessPermission(const content::MediaStreamRequest& request,
                                    content::MediaResponseCallback callback);
  void RequestPointerLockPermission(
      content::RenderFrameHost* frame,
      bool user_gesture,
      bool last_unlocked_by_target,
      base::OnceCallback<void(content::WebContents*, bool, bool, bool)>
          callback);
  void RequestKeyboardLockPermission(
      content::RenderFrameHost* requesting_frame,
      bool esc_key_locked,
      base::OnceCallback<void(content::WebContents*, bool, bool)> callback);
  void RequestWebNotificationPermission(
      content::RenderFrameHost* requesting_frame,
      base::OnceCallback<void(bool)> callback);
  void RequestOpenExternalPermission(content::RenderFrameHost* requesting_frame,
                                     base::OnceCallback<void(bool)> callback,
                                     bool user_gesture,
                                     const GURL& url);

  // Synchronous Checks
  bool CheckMediaAccessPermission(content::RenderFrameHost* requesting_frame,
                                  const url::Origin& security_origin,
                                  blink::mojom::MediaStreamType type) const;

 private:
  explicit WebContentsPermissionHelper(content::WebContents* web_contents);
  friend class content::WebContentsUserData<WebContentsPermissionHelper>;

  void RequestPermission(content::RenderFrameHost* requesting_frame,
                         const url::Origin& origin,
                         blink::PermissionType permission,
                         base::OnceCallback<void(bool)> callback,
                         bool user_gesture = false,
                         base::Value::Dict details = {});

  // TODO(clavin): refactor to use the WebContents provided by the
  // WebContentsUserData base class instead of storing a duplicate ref
  raw_ptr<content::WebContents> web_contents_;

  WEB_CONTENTS_USER_DATA_KEY_DECL();
};

}  // namespace electron

#endif  // ELECTRON_SHELL_BROWSER_WEB_CONTENTS_PERMISSION_HELPER_H_
