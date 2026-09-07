// Copyright (c) 2016 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_WEB_CONTENTS_PERMISSION_HELPER_H_
#define ELECTRON_SHELL_BROWSER_WEB_CONTENTS_PERMISSION_HELPER_H_

#include <optional>

#include "base/memory/raw_ptr.h"
#include "base/values.h"
#include "content/public/browser/media_stream_request.h"
#include "content/public/browser/web_contents_user_data.h"
#include "third_party/blink/public/common/mediastream/media_stream_request.h"
#include "third_party/blink/public/common/permissions/permission_utils.h"
#include "url/origin.h"

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

  // Asynchronous Requests
  void RequestFullscreenPermission(content::RenderFrameHost* requesting_frame,
                                   base::OnceCallback<void(bool)> callback);
  void RequestMediaAccessPermission(const content::MediaStreamRequest& request,
                                    content::MediaResponseCallback callback);
  void RequestPointerLockPermission(
      bool user_gesture,
      bool last_unlocked_by_target,
      base::OnceCallback<void(content::WebContents*, bool, bool, bool)>
          callback);
  void RequestKeyboardLockPermission(
      bool esc_key_locked,
      base::OnceCallback<void(content::WebContents*, bool, bool)> callback);
  void RequestWebNotificationPermission(
      content::RenderFrameHost* requesting_frame,
      base::OnceCallback<void(bool)> callback);
  // Identity to report for an external protocol launch when the initiating
  // document no longer exists: its origin (or, failing that, the origin
  // content holds responsible) and whether it was a main frame.
  struct ExternalProtocolRequester {
    ExternalProtocolRequester(const url::Origin& origin, bool is_main_frame)
        : origin(origin), is_main_frame(is_main_frame) {}
    url::Origin origin;
    bool is_main_frame;
  };

  // |requesting_frame| is the initiator document's frame, or the navigating
  // WebContents' main frame if that document is gone or the browser started
  // the navigation. With |requester| set, the request is reported with that
  // origin and main-frame-ness instead of |requesting_frame|'s.
  void RequestOpenExternalPermission(
      content::RenderFrameHost* requesting_frame,
      base::OnceCallback<void(bool)> callback,
      bool user_gesture,
      const GURL& url,
      const std::optional<ExternalProtocolRequester>& requester = std::nullopt);

  // Synchronous Checks
  bool CheckMediaAccessPermission(content::RenderFrameHost* requesting_frame,
                                  const url::Origin& security_origin,
                                  blink::mojom::MediaStreamType type) const;
  bool CheckSerialAccessPermission(
      content::RenderFrameHost* requesting_frame) const;

 private:
  explicit WebContentsPermissionHelper(content::WebContents* web_contents);
  friend class content::WebContentsUserData<WebContentsPermissionHelper>;

  void RequestPermission(content::RenderFrameHost* requesting_frame,
                         blink::PermissionType permission,
                         base::OnceCallback<void(bool)> callback,
                         bool user_gesture = false,
                         base::DictValue details = {});

  bool CheckPermission(content::RenderFrameHost* requesting_frame,
                       blink::PermissionType permission,
                       base::DictValue details) const;

  // TODO(clavin): refactor to use the WebContents provided by the
  // WebContentsUserData base class instead of storing a duplicate ref
  raw_ptr<content::WebContents> web_contents_;

  WEB_CONTENTS_USER_DATA_KEY_DECL();
};

}  // namespace electron

#endif  // ELECTRON_SHELL_BROWSER_WEB_CONTENTS_PERMISSION_HELPER_H_
