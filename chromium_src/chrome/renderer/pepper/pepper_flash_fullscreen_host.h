// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_RENDERER_PEPPER_PEPPER_FLASH_FULLSCREEN_HOST_H_
#define CHROME_RENDERER_PEPPER_PEPPER_FLASH_FULLSCREEN_HOST_H_

#include "base/basictypes.h"
#include "base/compiler_specific.h"
#include "ppapi/host/resource_host.h"

namespace content {
class RendererPpapiHost;
}

class PepperFlashFullscreenHost : public ppapi::host::ResourceHost {
 public:
  PepperFlashFullscreenHost(content::RendererPpapiHost* host,
                            PP_Instance instance,
                            PP_Resource resource);
  ~PepperFlashFullscreenHost() override;

  int32_t OnResourceMessageReceived(
      const IPC::Message& msg,
      ppapi::host::HostMessageContext* context) override;

 private:
  int32_t OnSetFullscreen(ppapi::host::HostMessageContext* context,
                          bool fullscreen);

  // Non-owning pointer.
  content::RendererPpapiHost* renderer_ppapi_host_;

  DISALLOW_COPY_AND_ASSIGN(PepperFlashFullscreenHost);
};

#endif  // CHROME_RENDERER_PEPPER_PEPPER_FLASH_FULLSCREEN_HOST_H_
