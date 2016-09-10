// Copyright (c) 2016 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef BRAVE_BROWSER_BRAVE_PERMISSION_MANAGER_H_
#define BRAVE_BROWSER_BRAVE_PERMISSION_MANAGER_H_

#include <map>
#include <vector>

#include "atom/browser/atom_permission_manager.h"
#include "base/callback.h"

namespace content {
class WebContents;
}

namespace brave {

class BravePermissionManager : public atom::AtomPermissionManager {
 public:
  BravePermissionManager();
  ~BravePermissionManager() override;

  using ResponseCallback =
      base::Callback<void(blink::mojom::PermissionStatus)>;
  using RequestHandler =
      base::Callback<void(const GURL&,
                          const GURL&,
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

 protected:
  void OnPermissionResponse(int request_id,
                            const GURL& url,
                            const ResponseCallback& callback,
                            blink::mojom::PermissionStatus status);

  // content::PermissionManager:
  void CancelPermissionRequest(int request_id) override;

 private:
  struct RequestInfo {
    int render_process_id;
    int render_frame_id;
    ResponseCallback callback;
  };

  RequestHandler request_handler_;

  std::map<int, RequestInfo> pending_requests_;

  int request_id_;

  DISALLOW_COPY_AND_ASSIGN(BravePermissionManager);
};

}  // namespace brave

#endif  // BRAVE_BROWSER_BRAVE_PERMISSION_MANAGER_H_
