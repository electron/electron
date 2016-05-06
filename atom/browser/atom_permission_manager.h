// Copyright (c) 2016 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_ATOM_PERMISSION_MANAGER_H_
#define ATOM_BROWSER_ATOM_PERMISSION_MANAGER_H_

#include <map>
#include <vector>

#include "base/callback.h"
#include "content/public/browser/permission_manager.h"

namespace content {
class WebContents;
}

namespace atom {

class AtomPermissionManager : public content::PermissionManager {
 public:
  AtomPermissionManager();
  ~AtomPermissionManager() override;

  using ResponseCallback =
      base::Callback<void(content::PermissionStatus)>;
  using RequestHandler =
      base::Callback<void(content::WebContents*,
                          content::PermissionType,
                          const ResponseCallback&)>;

  // Handler to dispatch permission requests in JS.
  void SetPermissionRequestHandler(const RequestHandler& handler);

  // content::PermissionManager:
  int RequestPermission(
      content::PermissionType permission,
      content::RenderFrameHost* render_frame_host,
      const GURL& requesting_origin,
      const ResponseCallback& callback) override;
  int RequestPermissions(
      const std::vector<content::PermissionType>& permissions,
      content::RenderFrameHost* render_frame_host,
      const GURL& requesting_origin,
      const base::Callback<void(
      const std::vector<content::PermissionStatus>&)>& callback) override;

 protected:
  void OnPermissionResponse(int request_id,
                            const GURL& url,
                            const ResponseCallback& callback,
                            content::PermissionStatus status);

  // content::PermissionManager:
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
  struct RequestInfo {
    int render_process_id;
    ResponseCallback callback;
  };

  RequestHandler request_handler_;

  std::map<int, RequestInfo> pending_requests_;

  int request_id_;

  DISALLOW_COPY_AND_ASSIGN(AtomPermissionManager);
};

}  // namespace atom

#endif  // ATOM_BROWSER_ATOM_PERMISSION_MANAGER_H_
