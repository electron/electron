// Copyright (c) 2016 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_WEB_CONTENTS_PERMISSION_HELPER_H_
#define ATOM_BROWSER_WEB_CONTENTS_PERMISSION_HELPER_H_

#include "atom/browser/atom_browser_context.h"
#include "content/public/browser/permission_type.h"
#include "content/public/browser/web_contents_user_data.h"
#include "content/public/common/media_stream_request.h"

namespace atom {

// Applies the permission requested for WebContents.
class WebContentsPermissionHelper
    : public content::WebContentsUserData<WebContentsPermissionHelper> {
 public:
  ~WebContentsPermissionHelper() override;

  void RequestMediaAccessPermission(
      const content::MediaStreamRequest& request,
      const content::MediaResponseCallback& callback);
  void RequestWebNotificationPermission(
      const base::Callback<void(bool)>& callback);

  AtomBrowserContext* browser_context() const {
    return static_cast<AtomBrowserContext*>(web_contents_->GetBrowserContext());
  }

 private:
  explicit WebContentsPermissionHelper(content::WebContents* web_contents);
  friend class content::WebContentsUserData<WebContentsPermissionHelper>;

  void RequestPermission(
      content::PermissionType permission,
      const base::Callback<void(bool)>& callback);

  content::WebContents* web_contents_;

  DISALLOW_COPY_AND_ASSIGN(WebContentsPermissionHelper);
};

}  // namespace atom

#endif  // ATOM_BROWSER_WEB_CONTENTS_PERMISSION_HELPER_H_
