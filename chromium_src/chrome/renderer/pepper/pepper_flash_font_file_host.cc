// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/renderer/pepper/pepper_flash_font_file_host.h"

#include "build/build_config.h"
#include "content/public/renderer/renderer_ppapi_host.h"
#include "ppapi/c/pp_errors.h"
#include "ppapi/host/dispatch_host_message.h"
#include "ppapi/host/host_message_context.h"
#include "ppapi/host/ppapi_host.h"
#include "ppapi/proxy/ppapi_messages.h"
#include "ppapi/proxy/serialized_structs.h"

#if defined(OS_LINUX) || defined(OS_OPENBSD)
#include "content/public/common/child_process_sandbox_support_linux.h"
#endif

PepperFlashFontFileHost::PepperFlashFontFileHost(
    content::RendererPpapiHost* host,
    PP_Instance instance,
    PP_Resource resource,
    const ppapi::proxy::SerializedFontDescription& description,
    PP_PrivateFontCharset charset)
    : ResourceHost(host->GetPpapiHost(), instance, resource) {
#if defined(OS_LINUX) || defined(OS_OPENBSD)
  fd_.reset(content::MatchFontWithFallback(
      description.face.c_str(),
      description.weight >= PP_BROWSERFONT_TRUSTED_WEIGHT_BOLD,
      description.italic,
      charset,
      PP_BROWSERFONT_TRUSTED_FAMILY_DEFAULT));
#endif  // defined(OS_LINUX) || defined(OS_OPENBSD)
}

PepperFlashFontFileHost::~PepperFlashFontFileHost() {}

int32_t PepperFlashFontFileHost::OnResourceMessageReceived(
    const IPC::Message& msg,
    ppapi::host::HostMessageContext* context) {
  PPAPI_BEGIN_MESSAGE_MAP(PepperFlashFontFileHost, msg)
    PPAPI_DISPATCH_HOST_RESOURCE_CALL(PpapiHostMsg_FlashFontFile_GetFontTable,
                                      OnGetFontTable)
  PPAPI_END_MESSAGE_MAP()
  return PP_ERROR_FAILED;
}

int32_t PepperFlashFontFileHost::OnGetFontTable(
    ppapi::host::HostMessageContext* context,
    uint32_t table) {
  std::string contents;
  int32_t result = PP_ERROR_FAILED;
#if defined(OS_LINUX) || defined(OS_OPENBSD)
  int fd = fd_.get();
  if (fd != -1) {
    size_t length = 0;
    if (content::GetFontTable(fd, table, 0 /* offset */, NULL, &length)) {
      contents.resize(length);
      uint8_t* contents_ptr =
          reinterpret_cast<uint8_t*>(const_cast<char*>(contents.c_str()));
      if (content::GetFontTable(
              fd, table, 0 /* offset */, contents_ptr, &length)) {
        result = PP_OK;
      } else {
        contents.clear();
      }
    }
  }
#endif  // defined(OS_LINUX) || defined(OS_OPENBSD)

  context->reply_msg = PpapiPluginMsg_FlashFontFile_GetFontTableReply(contents);
  return result;
}
