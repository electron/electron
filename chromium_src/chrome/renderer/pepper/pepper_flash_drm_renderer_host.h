// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_RENDERER_PEPPER_PEPPER_FLASH_DRM_RENDERER_HOST_H_
#define CHROME_RENDERER_PEPPER_PEPPER_FLASH_DRM_RENDERER_HOST_H_

#include "base/basictypes.h"
#include "base/compiler_specific.h"
#include "base/memory/weak_ptr.h"
#include "ppapi/host/resource_host.h"

namespace base {
class FilePath;
}

namespace content {
class RendererPpapiHost;
}

// TODO(raymes): This is only needed until we move FileRef resources to the
// browser. After that, get rid of this class altogether.
class PepperFlashDRMRendererHost : public ppapi::host::ResourceHost {
 public:
  PepperFlashDRMRendererHost(content::RendererPpapiHost* host,
                             PP_Instance instance,
                             PP_Resource resource);
  ~PepperFlashDRMRendererHost() override;

  int32_t OnResourceMessageReceived(
      const IPC::Message& msg,
      ppapi::host::HostMessageContext* context) override;

 private:
  int32_t OnGetVoucherFile(ppapi::host::HostMessageContext* context);

  void DidCreateFileRefHosts(
      const ppapi::host::ReplyMessageContext& reply_context,
      const base::FilePath& external_path,
      int renderer_pending_host_id,
      const std::vector<int>& browser_pending_host_ids);

  // Non-owning pointer.
  content::RendererPpapiHost* renderer_ppapi_host_;

  base::WeakPtrFactory<PepperFlashDRMRendererHost> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(PepperFlashDRMRendererHost);
};

#endif  // CHROME_RENDERER_PEPPER_PEPPER_FLASH_DRM_RENDERER_HOST_H_
