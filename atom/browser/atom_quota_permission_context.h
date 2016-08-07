// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#pragma once

#include "content/public/browser/quota_permission_context.h"

namespace atom {

class AtomQuotaPermissionContext : public content::QuotaPermissionContext {
 public:
  typedef content::QuotaPermissionContext::QuotaPermissionResponse response;

  AtomQuotaPermissionContext();
  virtual ~AtomQuotaPermissionContext();

  // content::QuotaPermissionContext:
  void RequestQuotaPermission(
      const content::StorageQuotaParams& params,
      int render_process_id,
      const PermissionCallback& callback) override;

 private:
  DISALLOW_COPY_AND_ASSIGN(AtomQuotaPermissionContext);
};

}  // namespace atom
