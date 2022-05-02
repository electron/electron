// Copyright (c) 2016 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_ELECTRON_PERMISSION_MANAGER_H_
#define ELECTRON_SHELL_BROWSER_ELECTRON_PERMISSION_MANAGER_H_

#include <memory>
#include <vector>

#include "base/callback.h"
#include "base/containers/id_map.h"
#include "content/public/browser/permission_controller_delegate.h"
#include "gin/dictionary.h"

namespace base {
class DictionaryValue;
class Value;
}  // namespace base

namespace content {
class WebContents;
}

namespace electron {

class ElectronPermissionManager : public content::PermissionControllerDelegate {
 public:
  ElectronPermissionManager();
  ~ElectronPermissionManager() override;

  // disable copy
  ElectronPermissionManager(const ElectronPermissionManager&) = delete;
  ElectronPermissionManager& operator=(const ElectronPermissionManager&) =
      delete;

  using StatusCallback =
      base::OnceCallback<void(blink::mojom::PermissionStatus)>;
  using StatusesCallback = base::OnceCallback<void(
      const std::vector<blink::mojom::PermissionStatus>&)>;
  using RequestHandler = base::RepeatingCallback<void(content::WebContents*,
                                                      blink::PermissionType,
                                                      StatusCallback,
                                                      const base::Value&)>;
  using CheckHandler =
      base::RepeatingCallback<bool(content::WebContents*,
                                   blink::PermissionType,
                                   const GURL& requesting_origin,
                                   const base::Value&)>;

  using DeviceCheckHandler =
      base::RepeatingCallback<bool(const v8::Local<v8::Object>&)>;

  // Handler to dispatch permission requests in JS.
  void SetPermissionRequestHandler(const RequestHandler& handler);
  void SetPermissionCheckHandler(const CheckHandler& handler);
  void SetDevicePermissionHandler(const DeviceCheckHandler& handler);

  // content::PermissionControllerDelegate:
  void RequestPermission(blink::PermissionType permission,
                         content::RenderFrameHost* render_frame_host,
                         const GURL& requesting_origin,
                         bool user_gesture,
                         StatusCallback callback) override;
  void RequestPermissionWithDetails(blink::PermissionType permission,
                                    content::RenderFrameHost* render_frame_host,
                                    const GURL& requesting_origin,
                                    bool user_gesture,
                                    const base::DictionaryValue* details,
                                    StatusCallback callback);
  void RequestPermissions(
      const std::vector<blink::PermissionType>& permissions,
      content::RenderFrameHost* render_frame_host,
      const GURL& requesting_origin,
      bool user_gesture,
      StatusesCallback callback) override;
  void RequestPermissionsWithDetails(
      const std::vector<blink::PermissionType>& permissions,
      content::RenderFrameHost* render_frame_host,
      const GURL& requesting_origin,
      bool user_gesture,
      const base::DictionaryValue* details,
      StatusesCallback callback);
  blink::mojom::PermissionStatus GetPermissionStatusForFrame(
      blink::PermissionType permission,
      content::RenderFrameHost* render_frame_host,
      const GURL& requesting_origin) override;
  blink::mojom::PermissionStatus GetPermissionStatusForCurrentDocument(
      blink::PermissionType permission,
      content::RenderFrameHost* render_frame_host) override;

  bool CheckPermissionWithDetails(blink::PermissionType permission,
                                  content::RenderFrameHost* render_frame_host,
                                  const GURL& requesting_origin,
                                  const base::DictionaryValue* details) const;

  bool CheckDevicePermission(blink::PermissionType permission,
                             const url::Origin& origin,
                             const base::Value* object,
                             content::RenderFrameHost* render_frame_host) const;

  void GrantDevicePermission(blink::PermissionType permission,
                             const url::Origin& origin,
                             const base::Value* object,
                             content::RenderFrameHost* render_frame_host) const;

 protected:
  void OnPermissionResponse(int request_id,
                            int permission_id,
                            blink::mojom::PermissionStatus status);

  // content::PermissionControllerDelegate:
  void ResetPermission(blink::PermissionType permission,
                       const GURL& requesting_origin,
                       const GURL& embedding_origin) override;
  blink::mojom::PermissionStatus GetPermissionStatus(
      blink::PermissionType permission,
      const GURL& requesting_origin,
      const GURL& embedding_origin) override;
  blink::mojom::PermissionStatus GetPermissionStatusForWorker(
      blink::PermissionType permission,
      content::RenderProcessHost* render_process_host,
      const GURL& worker_origin) override;
  SubscriptionId SubscribePermissionStatusChange(
      blink::PermissionType permission,
      content::RenderProcessHost* render_process_host,
      content::RenderFrameHost* render_frame_host,
      const GURL& requesting_origin,
      base::RepeatingCallback<void(blink::mojom::PermissionStatus)> callback)
      override;
  void UnsubscribePermissionStatusChange(SubscriptionId id) override;

 private:
  class PendingRequest;
  using PendingRequestsMap = base::IDMap<std::unique_ptr<PendingRequest>>;

  RequestHandler request_handler_;
  CheckHandler check_handler_;
  DeviceCheckHandler device_permission_handler_;

  PendingRequestsMap pending_requests_;
};

}  // namespace electron

#endif  // ELECTRON_SHELL_BROWSER_ELECTRON_PERMISSION_MANAGER_H_
