// Copyright (c) 2016 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef SHELL_BROWSER_ELECTRON_PERMISSION_MANAGER_H_
#define SHELL_BROWSER_ELECTRON_PERMISSION_MANAGER_H_

#include <map>
#include <memory>
#include <vector>

#include "base/callback.h"
#include "base/containers/id_map.h"
#include "base/values.h"
#include "content/public/browser/permission_controller_delegate.h"

namespace content {
class WebContents;
}

namespace electron {

class ElectronPermissionManager : public content::PermissionControllerDelegate {
 public:
  ElectronPermissionManager();
  ~ElectronPermissionManager() override;

  using StatusCallback =
      base::OnceCallback<void(blink::mojom::PermissionStatus)>;
  using StatusesCallback = base::OnceCallback<void(
      const std::vector<blink::mojom::PermissionStatus>&)>;
  using RequestHandler = base::Callback<void(content::WebContents*,
                                             content::PermissionType,
                                             StatusCallback,
                                             const base::Value&)>;
  using CheckHandler = base::Callback<bool(content::WebContents*,
                                           content::PermissionType,
                                           const GURL& requesting_origin,
                                           const base::Value&)>;

  // Handler to dispatch permission requests in JS.
  void SetPermissionRequestHandler(const RequestHandler& handler);
  void SetPermissionCheckHandler(const CheckHandler& handler);

  // content::PermissionControllerDelegate:
  int RequestPermission(content::PermissionType permission,
                        content::RenderFrameHost* render_frame_host,
                        const GURL& requesting_origin,
                        bool user_gesture,
                        StatusCallback callback) override;
  int RequestPermissionWithDetails(content::PermissionType permission,
                                   content::RenderFrameHost* render_frame_host,
                                   const GURL& requesting_origin,
                                   bool user_gesture,
                                   const base::DictionaryValue* details,
                                   StatusCallback callback);
  int RequestPermissions(
      const std::vector<content::PermissionType>& permissions,
      content::RenderFrameHost* render_frame_host,
      const GURL& requesting_origin,
      bool user_gesture,
      StatusesCallback callback) override;
  int RequestPermissionsWithDetails(
      const std::vector<content::PermissionType>& permissions,
      content::RenderFrameHost* render_frame_host,
      const GURL& requesting_origin,
      bool user_gesture,
      const base::DictionaryValue* details,
      StatusesCallback callback);
  blink::mojom::PermissionStatus GetPermissionStatusForFrame(
      content::PermissionType permission,
      content::RenderFrameHost* render_frame_host,
      const GURL& requesting_origin) override;

  bool CheckPermissionWithDetails(content::PermissionType permission,
                                  content::RenderFrameHost* render_frame_host,
                                  const GURL& requesting_origin,
                                  const base::DictionaryValue* details) const;

 protected:
  void OnPermissionResponse(int request_id,
                            int permission_id,
                            blink::mojom::PermissionStatus status);

  // content::PermissionControllerDelegate:
  void ResetPermission(content::PermissionType permission,
                       const GURL& requesting_origin,
                       const GURL& embedding_origin) override;
  blink::mojom::PermissionStatus GetPermissionStatus(
      content::PermissionType permission,
      const GURL& requesting_origin,
      const GURL& embedding_origin) override;
  SubscriptionId SubscribePermissionStatusChange(
      content::PermissionType permission,
      content::RenderFrameHost* render_frame_host,
      const GURL& requesting_origin,
      base::RepeatingCallback<void(blink::mojom::PermissionStatus)> callback)
      override;
  void UnsubscribePermissionStatusChange(
      SubscriptionId subscription_id) override;

 private:
  class PendingRequest;
  using PendingRequestsMap = base::IDMap<std::unique_ptr<PendingRequest>>;

  RequestHandler request_handler_;
  CheckHandler check_handler_;

  PendingRequestsMap pending_requests_;

  DISALLOW_COPY_AND_ASSIGN(ElectronPermissionManager);
};

}  // namespace electron

#endif  // SHELL_BROWSER_ELECTRON_PERMISSION_MANAGER_H_
