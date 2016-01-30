// Copyright (c) 2016 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_ATOM_PERMISSION_MANAGER_H_
#define ATOM_BROWSER_ATOM_PERMISSION_MANAGER_H_

#include <map>

#include "base/callback_forward.h"
#include "base/macros.h"
#include "content/public/browser/permission_manager.h"

namespace atom {

class AtomPermissionManager : public content::PermissionManager {
 public:
  AtomPermissionManager();
  ~AtomPermissionManager() override;

  using ResponseCallback =
      base::Callback<void(content::PermissionStatus)>;
  using RequestHandler =
      base::Callback<void(content::PermissionType,
                          const ResponseCallback&)>;

  void SetPermissionRequestHandler(int id, const RequestHandler& handler);
  void RequestPermission(
      content::PermissionType permission,
      content::RenderFrameHost* render_frame_host,
      const GURL& origin,
      const base::Callback<void(bool)>& callback);
  void OnPermissionResponse(
      const base::Callback<void(bool)>& callback,
      content::PermissionStatus status);

 protected:
  // content::PermissionManager:
  int RequestPermission(
      content::PermissionType permission,
      content::RenderFrameHost* render_frame_host,
      const GURL& requesting_origin,
      bool user_gesture,
      const ResponseCallback& callback) override;
  void CancelPermissionRequest(int request_id) override;
  void ResetPermission(content::PermissionType permission,
                       const GURL& requesting_origin,
                       const GURL& embedding_origin) override;
  content::PermissionStatus GetPermissionStatus(
      content::PermissionType permission,
      const GURL& requesting_origin,
      const GURL& embedding_origin) override;
  void RegisterPermissionUsage(content::PermissionType permission,
                               const GURL& requesting_origin,
                               const GURL& embedding_origin) override;
  int SubscribePermissionStatusChange(
      content::PermissionType permission,
      const GURL& requesting_origin,
      const GURL& embedding_origin,
      const base::Callback<void(content::PermissionStatus)>& callback) override;
  void UnsubscribePermissionStatusChange(int subscription_id) override;

 private:
  std::map<int, RequestHandler> request_handler_map_;

  std::map<int, ResponseCallback> pending_requests_;

  int request_id_;

  DISALLOW_COPY_AND_ASSIGN(AtomPermissionManager);
};

}  // namespace atom

#endif  // ATOM_BROWSER_ATOM_PERMISSION_MANAGER_H_
