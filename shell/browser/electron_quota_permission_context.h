// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_ELECTRON_QUOTA_PERMISSION_CONTEXT_H_
#define ELECTRON_SHELL_BROWSER_ELECTRON_QUOTA_PERMISSION_CONTEXT_H_

#include "content/public/browser/quota_permission_context.h"

namespace content {
struct StorageQuotaParams;
}

namespace electron {

class ElectronQuotaPermissionContext : public content::QuotaPermissionContext {
 public:
  typedef content::QuotaPermissionContext::QuotaPermissionResponse response;

  ElectronQuotaPermissionContext();

  // disable copy
  ElectronQuotaPermissionContext(const ElectronQuotaPermissionContext&) =
      delete;
  ElectronQuotaPermissionContext& operator=(
      const ElectronQuotaPermissionContext&) = delete;

  // content::QuotaPermissionContext:
  void RequestQuotaPermission(const content::StorageQuotaParams& params,
                              int render_process_id,
                              PermissionCallback callback) override;

 private:
  ~ElectronQuotaPermissionContext() override;
};

}  // namespace electron

#endif  // ELECTRON_SHELL_BROWSER_ELECTRON_QUOTA_PERMISSION_CONTEXT_H_
