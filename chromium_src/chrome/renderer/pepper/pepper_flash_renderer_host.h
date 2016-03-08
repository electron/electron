// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_RENDERER_PEPPER_PEPPER_FLASH_RENDERER_HOST_H_
#define CHROME_RENDERER_PEPPER_PEPPER_FLASH_RENDERER_HOST_H_

#include <string>
#include <vector>

#include "base/memory/weak_ptr.h"
#include "ppapi/host/host_message_context.h"
#include "ppapi/host/resource_host.h"

struct PP_Rect;

namespace ppapi {
struct URLRequestInfoData;
}

namespace ppapi {
namespace proxy {
struct PPBFlash_DrawGlyphs_Params;
}
}

namespace content {
class RendererPpapiHost;
}

class PepperFlashRendererHost : public ppapi::host::ResourceHost {
 public:
  PepperFlashRendererHost(content::RendererPpapiHost* host,
                          PP_Instance instance,
                          PP_Resource resource);
  ~PepperFlashRendererHost() override;

  // ppapi::host::ResourceHost override.
  int32_t OnResourceMessageReceived(
      const IPC::Message& msg,
      ppapi::host::HostMessageContext* context) override;

 private:
  int32_t OnGetProxyForURL(ppapi::host::HostMessageContext* host_context,
                           const std::string& url);
  int32_t OnSetInstanceAlwaysOnTop(
      ppapi::host::HostMessageContext* host_context,
      bool on_top);
  int32_t OnDrawGlyphs(ppapi::host::HostMessageContext* host_context,
                       ppapi::proxy::PPBFlash_DrawGlyphs_Params params);
  int32_t OnNavigate(ppapi::host::HostMessageContext* host_context,
                     const ppapi::URLRequestInfoData& data,
                     const std::string& target,
                     bool from_user_action);
  int32_t OnIsRectTopmost(ppapi::host::HostMessageContext* host_context,
                          const PP_Rect& rect);

  // A stack of ReplyMessageContexts to track Navigate() calls which have not
  // yet been replied to.
  std::vector<ppapi::host::ReplyMessageContext> navigate_replies_;

  content::RendererPpapiHost* host_;
  base::WeakPtrFactory<PepperFlashRendererHost> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(PepperFlashRendererHost);
};

#endif  // CHROME_RENDERER_PEPPER_PEPPER_FLASH_RENDERER_HOST_H_
