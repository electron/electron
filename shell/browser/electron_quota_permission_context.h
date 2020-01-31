// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef SHELL_BROWSER_ELECTRON_QUOTA_PERMISSION_CONTEXT_H_
#define SHELL_BROWSER_ELECTRON_QUOTA_PERMISSION_CONTEXT_H_

#include "content/public/browser/quota_permission_context.h"
#include "content/public/common/storage_quota_params.h"

namespace electron {

class ElectronQuotaPermissionContext : public content::QuotaPermissionContext {
 public:
  typedef content::QuotaPermissionContext::QuotaPermissionResponse response;

  ElectronQuotaPermissionContext();

  // content::QuotaPermissionContext:
  void RequestQuotaPermission(const content::StorageQuotaParams& params,
                              int render_process_id,
                              PermissionCallback callback) override;

 private:
  ~ElectronQuotaPermissionContext() override;

  DISALLOW_COPY_AND_ASSIGN(ElectronQuotaPermissionContext);
};

}  // namespace electron

#endif  // SHELL_BROWSER_ELECTRON_QUOTA_PERMISSION_CONTEXT_H_
