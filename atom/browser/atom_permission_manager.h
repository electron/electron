// Copyright (c) 2016 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_ATOM_PERMISSION_MANAGER_H_
#define ATOM_BROWSER_ATOM_PERMISSION_MANAGER_H_

#include <map>
#include <vector>

#include "base/callback.h"
#include "base/id_map.h"
#include "base/values.h"
#include "content/public/browser/permission_manager.h"

namespace content {
class WebContents;
}

namespace atom {

class AtomPermissionManager : public content::PermissionManager {
 public:
  AtomPermissionManager();
  ~AtomPermissionManager() override;

  using StatusCallback =
      base::Callback<void(blink::mojom::PermissionStatus)>;
  using StatusesCallback =
      base::Callback<void(const std::vector<blink::mojom::PermissionStatus>&)>;
  using RequestHandler =
      base::Callback<void(content::WebContents*,
                          content::PermissionType,
                          const StatusCallback&,
                          const base::DictionaryValue&)>;

  // Handler to dispatch permission requests in JS.
  void SetPermissionRequestHandler(const RequestHandler& handler);

  // content::PermissionManager:
  int RequestPermission(
      content::PermissionType permission,
      content::RenderFrameHost* render_frame_host,
      const GURL& requesting_origin,
      bool user_gesture,
      const base::Callback<void(blink::mojom::PermissionStatus)>& callback)
      override;
  int RequestPermissionWithDetails(
      content::PermissionType permission,
      content::RenderFrameHost* render_frame_host,
      const GURL& requesting_origin,
      bool user_gesture,
      const base::DictionaryValue* details,
      const base::Callback<void(blink::mojom::PermissionStatus)>& callback);
  int RequestPermissions(
      const std::vector<content::PermissionType>& permissions,
      content::RenderFrameHost* render_frame_host,
      const GURL& requesting_origin,
      bool user_gesture,
      const base::Callback<void(
          const std::vector<blink::mojom::PermissionStatus>&)>& callback)
      override;
  int RequestPermissionsWithDetails(
      const std::vector<content::PermissionType>& permissions,
      content::RenderFrameHost* render_frame_host,
      const GURL& requesting_origin,
      bool user_gesture,
      const base::DictionaryValue* details,
      const base::Callback<void(
          const std::vector<blink::mojom::PermissionStatus>&)>& callback);

 protected:
  void OnPermissionResponse(int request_id,
                            int permission_id,
                            blink::mojom::PermissionStatus status);

  // content::PermissionManager:
  void CancelPermissionRequest(int request_id) override;
  void ResetPermission(content::PermissionType permission,
                       const GURL& requesting_origin,
                       const GURL& embedding_origin) override;
  blink::mojom::PermissionStatus GetPermissionStatus(
      content::PermissionType permission,
      const GURL& requesting_origin,
      const GURL& embedding_origin) override;
  int SubscribePermissionStatusChange(
      content::PermissionType permission,
      const GURL& requesting_origin,
      const GURL& embedding_origin,
      const base::Callback<void(blink::mojom::PermissionStatus)>& callback)
      override;
  void UnsubscribePermissionStatusChange(int subscription_id) override;

 private:
  class PendingRequest;
  using PendingRequestsMap = IDMap<std::unique_ptr<PendingRequest>>;

  RequestHandler request_handler_;

  PendingRequestsMap pending_requests_;

  DISALLOW_COPY_AND_ASSIGN(AtomPermissionManager);
};

}  // namespace atom

#endif  // ATOM_BROWSER_ATOM_PERMISSION_MANAGER_H_
