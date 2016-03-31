// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_BROWSER_ELECTRON_QUOTA_PERMISSION_CONTEXT_H_
#define ELECTRON_BROWSER_ELECTRON_QUOTA_PERMISSION_CONTEXT_H_

#include "content/public/browser/quota_permission_context.h"

namespace electron {

class ElectronQuotaPermissionContext : public content::QuotaPermissionContext {
 public:
  typedef content::QuotaPermissionContext::QuotaPermissionResponse response;

  ElectronQuotaPermissionContext();
  virtual ~ElectronQuotaPermissionContext();

  // content::QuotaPermissionContext:
  void RequestQuotaPermission(
      const content::StorageQuotaParams& params,
      int render_process_id,
      const PermissionCallback& callback) override;

 private:
  DISALLOW_COPY_AND_ASSIGN(ElectronQuotaPermissionContext);
};

}  // namespace electron

#endif  // ELECTRON_BROWSER_ELECTRON_QUOTA_PERMISSION_CONTEXT_H_
