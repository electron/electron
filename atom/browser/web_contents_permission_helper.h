// Copyright (c) 2016 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_WEB_CONTENTS_PERMISSION_HELPER_H_
#define ATOM_BROWSER_WEB_CONTENTS_PERMISSION_HELPER_H_

#include <map>
#include <string>

#include "base/callback.h"
#include "content/public/browser/web_contents_user_data.h"
#include "content/public/common/media_stream_request.h"

namespace atom {

namespace api {
class WebContents;
}

// Applies the permission requested for WebContents.
class WebContentsPermissionHelper
    : public content::WebContentsUserData<WebContentsPermissionHelper> {
 public:
  WebContentsPermissionHelper(content::WebContents* web_contents,
                              api::WebContents* api_web_contents);
  ~WebContentsPermissionHelper() override;

  void RequestMediaAccessPermission(
      const content::MediaStreamRequest& request,
      const content::MediaResponseCallback& callback);
  void RequestWebNotificationPermission(
      const base::Callback<void(bool)>& callback);

  void OnPermissionResponse(const std::string& permission, bool allowed);

 private:
  friend class content::WebContentsUserData<WebContentsPermissionHelper>;

  std::map<std::string, base::Callback<void(bool)>> permission_map_;

  api::WebContents* api_web_contents_;  // Weak reference

  DISALLOW_COPY_AND_ASSIGN(WebContentsPermissionHelper);
};

}  // namespace atom

#endif  // ATOM_BROWSER_WEB_CONTENTS_PERMISSION_HELPER_H_
