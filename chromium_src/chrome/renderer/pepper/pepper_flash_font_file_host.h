// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_RENDERER_PEPPER_PEPPER_FLASH_FONT_FILE_HOST_H_
#define CHROME_RENDERER_PEPPER_PEPPER_FLASH_FONT_FILE_HOST_H_

#include "ppapi/c/private/pp_private_font_charset.h"
#include "ppapi/host/resource_host.h"

#if defined(OS_LINUX) || defined(OS_OPENBSD)
#include "base/files/scoped_file.h"
#endif

namespace content {
class RendererPpapiHost;
}

namespace ppapi {
namespace proxy {
struct SerializedFontDescription;
}
}

class PepperFlashFontFileHost : public ppapi::host::ResourceHost {
 public:
  PepperFlashFontFileHost(
      content::RendererPpapiHost* host,
      PP_Instance instance,
      PP_Resource resource,
      const ppapi::proxy::SerializedFontDescription& description,
      PP_PrivateFontCharset charset);
  ~PepperFlashFontFileHost() override;

  int32_t OnResourceMessageReceived(
      const IPC::Message& msg,
      ppapi::host::HostMessageContext* context) override;

 private:
  int32_t OnGetFontTable(ppapi::host::HostMessageContext* context,
                         uint32_t table);

#if defined(OS_LINUX) || defined(OS_OPENBSD)
  base::ScopedFD fd_;
#endif

  DISALLOW_COPY_AND_ASSIGN(PepperFlashFontFileHost);
};

#endif  // CHROME_RENDERER_PEPPER_PEPPER_FLASH_FONT_FILE_HOST_H_
